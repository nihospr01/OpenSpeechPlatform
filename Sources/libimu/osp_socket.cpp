/**
 * Author:    Anshuman Dewangan <adewangan@ucsd.edu>
 * Created:   04/30/2021
 **/

#include <osp_socket.hpp>

/////////////////////////////////
// General Socket Functions
/////////////////////////////////

int connect_socket() {
    /* Purpose: Connects to TCP socket on port PORT (default: 8001)
     *
     * Returns:
     *   - sock: socket for reading or sending messages
     *
     * Source: https://www.geeksforgeeks.org/socket-programming-cc/
     */
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return sock;
}

std::string send_messsage(json json_messsage) {
    /* Purpose: Sends a messsage through TCP socket
     *
     * Params:
     *   - json_messsage (json): json object to send
     * Returns:
     *   - response (string): character response from message
     */

    // Connect to the socket
    int sock = connect_socket();
    // Convert json_message to string for sending
    const std::string str_messsage = json_messsage.dump();

    // Send message
    send(sock, str_messsage.c_str(), strlen(str_messsage.c_str()), 0);
    printf("\nMessage sent\n");

    // Read response
    char response[8192] = {0};
    read(sock, response, 8192);
    return response;
}

std::string get_messsage() {
    /* Purpose: "get" from osp
     *
     * Returns:
     *   - response (string): response to "get"
     */
    json json_messsage = "{\"method\": \"get\"}"_json;
    return send_messsage(json_messsage);
}

/////////////////////////////////
// IMU-specific functions
/////////////////////////////////

void toggle_beamforming() {
    /* Purpose: Turn beamforming on or off */

    // Get current status of osp
    const std::string response1 = get_messsage();

    if (response1.length() == 0) {
        printf("no response from osp\n");
        return;
    }

    json json_response = json::parse(response1);

    // Set bf to 1 if off and to 0 if on and play audio cue
    json json_messsage;
    if (json_response["left"]["bf_imu_on_off"] == 1) {
        if (json_response["left"]["bf"] == 0)
            json_messsage =
                "{\"method\": \"set\", \"data\": {\"left\": {\"bf\": 1, \"audio_filename\": \"audio/dtmf_19.wav\", \"audio_alpha\": 0.5, \"audio_play\": 1}}}"_json;
        else if (json_response["left"]["bf"] == 1)
            json_messsage =
                "{\"method\": \"set\", \"data\": {\"left\": {\"bf\": 0, \"audio_filename\": \"audio/dtmf_91.wav\", \"audio_alpha\": 0.5, \"audio_play\": 1}}}"_json;

        // Send message
        const std::string response2 = send_messsage(json_messsage);
        printf("%s\n", response2.c_str());
    } else if (json_response["left"]["bf_imu_on_off"] == 0) {
        printf("\nIMU gestures not enabled\n");
    } else {
        printf("Unable to parse osp params\n");
    }
}
