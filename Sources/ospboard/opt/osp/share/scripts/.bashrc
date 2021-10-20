# .bashrc

export OSP_MEDIA=/opt/release/media
export OSP_REC=/opt/recordings
export LD_LIBRARY_PATH=/opt/release/lib

# If not running interactively, just quit now
[[ $- == *i* ]] || return

export TERM=ansi
resize &> /dev/null

# fancy prompt stuff
if [ -n "$PS1" ]; then
    txtred='\[\e[31m\]' # red
    txtgreen='\[\e[32m\]'   # green
    txtcyn='\[\e[0;96m\]' # Cyan
    txtpur='\[\e[0;35m\]' # Purple
    txtwht='\[\e[0;37m\]' # White
    txtrst='\[\e[0m\]'    # Text Reset
    # Which Color for what part of the prompt?
    pathC="${txtgreen}"
    promptC="${txtwht}"
    normalC="${txtrst}"
    hostC="${txtwht}"

    export HISTIGNORE="&:ls:[bf]g:exit:cd ..:cd"
    export HISTCONTROL=erasedups
    export HISTSIZE=10000
    shopt -s histappend cdspell cmdhist
    # After each command, append to the history file and reread it
    export PROMPT_COMMAND="${PROMPT_COMMAND:+$PROMPT_COMMAND$"\n"}history -a; history -c; history -r"

    if [ -f /etc/bash_completion ]; then
        . /etc/bash_completion
    fi
    export PS1="${debian_chroot:+($debian_chroot)}\u@${hostC}\h:${pathC}\w${promptC}> ${normalC}"
    alias ps_osp="ps -T -p \`pgrep OSP\` -o cpuid,cls,pri,pcpu,lwp,comm"
    alias ps_imu="pgrep IMU | xargs ps -T -o cpuid,cls,pri,pcpu,lwp,comm -p"
fi

export EDITOR=emacs

# management fuctions
function set_mode () {
    case $1 in
        hotspot | Hotspot | HotSpot | hs | HS)
            if [ -f "/root/.hsmode" ]; then
                    echo "Already in Hotspot mode, doing nothing..."
                    return 0
            fi
            echo "Switching to Hotspot mode and rebooting"
            systemctl enable dnsmasq &> /dev/null
            systemctl disable NetworkManager &> /dev/null
            cp -f /etc/network/interfaces.hs /etc/network/interfaces &> /dev/null
            touch /root/.hsmode &> /dev/null
            reboot
            ;;

        NetworkManager | nm | NM)
            if [ ! -f "/root/.hsmode" ]; then
                    echo "Already in NetworkManager mode, doing nothing..."
                    return 0
            fi
            echo "Switching to NetworkManager mode and rebooting"
            systemctl disable dnsmasq &> /dev/null
            systemctl enable NetworkManager &> /dev/null
            cp -f /etc/network/interfaces.nm /etc/network/interfaces &> /dev/null
            rm -f /root/.hsmode &> /dev/null
            reboot
            ;;

        *)
            echo
            echo "available options are"
            echo "Hotspot (hs) or NetworkManager (nm)"
            echo
            ;;
    esac
}

function print_mode () {
    if systemctl status NetworkManager > /dev/null; then
        printf "NetworkManager Mode\n"
        if ifconfig wlan0| grep "inet "
        then 
            printf "Connected\n"
        else
            printf "Connect to your wifi with one of the following:\n"
            printf "> nmcli dev wifi con MYSSID password \"MY PASSWORD\"\n"
            printf "or\n> nmtui\n"
            nmcli dev wifi list
        fi
        printf "If you wish to put the board back into hotspot mode then use following command:\n\nset_mode hs\n"
    else
        printf "Hotspot Mode\n"
        printf "Connect to the device at `grep ^ssid= /etc/hostapd/hostapd.conf` with `grep ^wpa_pass /etc/hostapd/hostapd.conf`\n"
        printf "open a browser and connect to http://192.168.8.1:5000 and/or http://192.168.8.1\n"
        printf "If you wish to put the board back into NetworkManager mode then use following command:\n\nset_mode nm\n"
    fi
}

# bash completions
if [ -f "/root/.hsmode" ]; then
    complete -W "NetworkManager nm" set_mode
else
    complete -W "Hotspot hs" set_mode
fi

function arec () {
    if [ $# -eq 0 ]
    then
        echo "Usage: arec {minutes}"
        echo "Audio is saved to \"recording.wav\""
    else
        let secs=$1*60
        printf "${secs}\n\n"
        arecord -D plughw:0,2 -r 48000 -f S16_LE -d ${secs} /opt/recordings/case_mics.wav
    fi
}

sleep 5
printf "\nOSP VERSION: "
cat /opt/osp_version
echo
print_mode

x=`timedatectl show | grep Timezone`
if [ $? -eq 0 ]; then
    if [ "$x" == "Timezone=Etc/UTC" ]; then 
	    printf "\nTo set timezone: \"timedatectl set-timezone America/Los_Angeles\"\n"
	    printf "To list timezones: \"timedatectl list-timezones\"\n\n" 
    fi
else
    hwclock -w
    printf "\nTo set timezone: \"timedatectl set-timezone America/Los_Angeles\"\n"
    printf "To list timezones: \"timedatectl list-timezones\"\n\n" 
fi
