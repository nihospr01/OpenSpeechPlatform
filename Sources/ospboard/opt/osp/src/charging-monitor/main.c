#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

#ifndef I2C_M_RD
#include <linux/i2c.h>
#endif

typedef unsigned char u8;

const char *ChgType[] = {
    "Nothing Attached",
    "SDP",
    "CDP",
    "DCP",
    "Apple 500mA",
    "Apple 1A",
    "Apple 2A",
    "Non-standard 90mA)",
    "Other",
    "unknown",
    "unknown",
    "unknown",
    "unknown",
    "unknown",
    "unknown",
    "unknown",
};

const char *BatDet[] = {"No Batt", "Battery"};

const char *ChgStat[] = {"Off",
                         "Supended due to overtemp",
                         "Precharging",
                         "Fast charge constant-current",
                         "Fast charge constant-voltage",
                         "Maintain charge",
                         "Maintain charge done",
                         "Charger fault"};

const u8 PRECHARGE_CONTROL_REG = 0x10; // Precharge Control Register
const u8 CHARGE_CONTROL_REG_A = 0x0B;  // sets fast-charge limit
const u8 CHARGE_CONTROL_REG_B = 0x0C;
const u8 CURRENT_CONTROL_REG = 0x0A;
const u8 CHARGING_ENABLED = 0x54;
const u8 STATUS_REG_A = 0x02;

const u8 LIMIT_AUTO = 0x0;

const u8 FC_LIMIT_200 = 0x08;
const u8 FC_LIMIT_450 = 0x1C;
const u8 FC_LIMIT_600 = 0x0C;
const u8 FC_LIMIT_AUTO = 0;

enum temp_state { TEMP_LOW, TEMP_MED, TEMP_HI };

int i2c_fd;

int open_i2c() {
    if ((i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        printf("Likely not V9 hardware.  Exiting charging-monitor.\n");
        return -1;
    }
    return 0;
}

u8 read_i2c(u8 reg) {
    u8 outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = 0x02;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = 0x02;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return 0;
    }
    return inbuf[0];
}

int write_i2c(u8 reg, u8 data) {
    u8 outbuf[2];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;

    msgs[0].addr = 0x02;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0)
        perror("ioctl(I2C_RDWR) in i2c_write");

    return 0;
}

// https://datasheets.maximintegrated.com/en/ds/MAX14746-MAX14747.pdf
int isCharging(u8 status) {
    u8 chargeStatus = status & 0x07;
    if (chargeStatus == 0 || chargeStatus == 1 || chargeStatus == 7)
        return 0;
    return 1;
}

// int is_SDP = 0;

void print_status(u8 status) {
    u8 chgstat = status & 0x07;
    u8 batdet = (status >> 3) & 1;
    u8 chgtype = (status >> 4) & 0xf;
    // is_SDP = (chgtype == 1) ? 1 : 0;
    printf("%s\t%s\t%s\n", BatDet[batdet], ChgType[chgtype], ChgStat[chgstat]);
}

void ledState(int temp) {
    FILE *green = fopen("/sys/class/leds/ospboard:green:sd/trigger", "a");
    FILE *red = fopen("/sys/class/leds/ospboard:green:kern/trigger", "a");
    // FILE *yellow = fopen("/sys/class/leds/ospboard:yellow:wlan/trigger", "a");
    // fprintf(yellow, is_SDP ? "default-on" : "phy0tx");

    if (temp == TEMP_LOW) {
        fprintf(green, "heartbeat");
        fprintf(red, "none");
    } else if (temp == TEMP_MED) {
        fprintf(green, "none");
        fprintf(red, "heartbeat");
    } else {
        fprintf(green, "none");
        fprintf(red, "default-on");
    }
    fclose(green);
    fclose(red);
    // fclose(yellow);
}

void sleep_secs(int seconds) {
    struct timespec requested, remaining;
    requested.tv_sec = seconds;
    requested.tv_nsec = 0;

    if (nanosleep(&requested, &remaining) < 0) {
        perror("Nano sleep system call failed");
        return;
    }
}

int read_temp() {
    int temp;
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    fscanf(f, "%d", &temp);
    fclose(f);
    return (int)(temp / 1000);
}

int main(int argc, char *argv[]) {

    u8 enabled = 0;
    u8 status = 0;
    enum temp_state new_state, old_state = TEMP_LOW;

    // disable buffering of stdout
    setbuf(stdout, NULL);

    if (open_i2c())
        return -1;

    printf("Setting initial current limit to AUTO\n");
    write_i2c(CURRENT_CONTROL_REG, LIMIT_AUTO);
    write_i2c(CHARGE_CONTROL_REG_A, FC_LIMIT_AUTO);
    write_i2c(PRECHARGE_CONTROL_REG,
              0xFF); // precharge 100mA, threshold=3V, BatDet state does not affect charge behavior

    while (1) {
        u8 stateChange = 0;

        sleep_secs(10);

        // check for charging enabled
        u8 value = read_i2c(CHARGE_CONTROL_REG_B);
        if (value != enabled) {
            switch (value) {
            case 0x54:
                printf("Charging Enabled\n");
                break;
            case 0xE4:
                printf("Charging was Disabled\n");
                break;
            default:
                printf("enabled state change %02X\n", value);
            }
            enabled = value;
            stateChange = 1;
        }

        // check temp
        int temp = read_temp();
        if (temp < 65) {
            new_state = TEMP_LOW;
        } else if (temp < 72) {
            new_state = TEMP_MED;
        } else {
            new_state = TEMP_HI;
        }

        // charging status
        value = read_i2c(STATUS_REG_A);
        if (new_state != old_state || value != status)
            stateChange = 1;

        if (value != status) {
            print_status(value);
            status = value;
        }

        if (stateChange) {
            time_t ltime = time(NULL);
            char *tm = asctime(localtime(&ltime));
            tm[strlen(tm) - 1] = 0;
            old_state = new_state;
            ledState(new_state);
            switch (new_state) {
            case TEMP_LOW:
            case TEMP_MED:
                if (enabled != CHARGING_ENABLED) {
                    write_i2c(CHARGE_CONTROL_REG_B, CHARGING_ENABLED);
                    printf("%s: Temp: %d. Changing enabled\n", tm, temp);
                }
                break;
            case TEMP_HI:
                if (enabled == CHARGING_ENABLED) {
                    write_i2c(CHARGE_CONTROL_REG_B, 0xE4);
                    printf("%s: Temp: %d. Charging disabled\n", tm, temp);
                }
                break;
            }
        }
    }
}
