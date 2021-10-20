#!/bin/bash

#Volume levels
ADC_VOL=4
DIG_ADC_VOL=200
DIG_DAC_VOL=200
amixer set 'ADC1' $ADC_VOL
amixer set 'ADC2' $ADC_VOL
amixer set 'ADC3' $ADC_VOL
amixer set 'TX1 Digital' $DIG_ADC_VOL
amixer set 'TX2 Digital' $DIG_ADC_VOL
amixer set 'RX1 Digital' $DIG_DAC_VOL
amixer set 'RX2 Digital' $DIG_DAC_VOL
amixer set 'RX3 Digital' $DIG_DAC_VOL

#ADC path routing
amixer set 'CIC1 MUX' 'AMIC'
amixer set 'CIC2 MUX' 'AMIC'
amixer set 'ADC2 MUX' 'INP3'
amixer set 'DEC1 MUX' 'ADC1'
amixer set 'DEC2 MUX' 'ADC2'

#DAC path routing
amixer set 'RDAC2 MUX' 'RX2'
amixer set 'HPHL' 'Switch'
amixer set 'HPHR' 'Switch'
amixer set 'RX1 MIX1 INP1' 'RX1'
amixer set 'RX1 MIX1 INP2' 'ZERO'
amixer set 'RX1 MIX1 INP3' 'ZERO'
amixer set 'RX2 MIX1 INP1' 'RX2'
amixer set 'RX2 MIX1 INP2' 'ZERO'
amixer set 'RX2 MIX1 INP3' 'ZERO'
amixer set 'RX3 MIX1 INP1' 'ZERO'
amixer set 'RX3 MIX1 INP2' 'ZERO'
amixer set 'RX3 MIX1 INP3' 'ZERO'

#ADC path options
amixer set 'TX1 HPF' off
amixer set 'TX2 HPF' off
amixer set 'TX1 HPF Cutoff' '4Hz'
amixer set 'TX2 HPF Cutoff' '4Hz'

#DAC path options
amixer set 'RX1 DCB' off
amixer set 'RX2 DCB' off
amixer set 'RX3 DCB' off
amixer set 'RX1 DCB Cutoff' '4Hz'
amixer set 'RX2 DCB Cutoff' '4Hz'
amixer set 'RX3 DCB Cutoff' '4Hz'
amixer set 'RX1 Mute' off
amixer set 'RX2 Mute' off
amixer set 'RX3 Mute' off

#Misc.
amixer set 'SPK DAC' off

