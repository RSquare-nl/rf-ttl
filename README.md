# rf-ttl
# E32-ttl-100 E36-ttl-100
This program can setup the modules E32 and E36 and probably others also.

To program you have to set the M0 and M1 to 1
In combination with the module E15-USB-tl this is easly done with jumpers, remove the jumpers M0 and M1 then you can program the device.

#Compile:
make

#To Run use:
./program-rf-ttl -d /dev/ttyUSB0 -a7 -s7 -o


#Options you can use:
- -d <serialdevice> serial port
- -o   Write permanent
- -f <0-1> FEC
- -w   Waiting time
- -c <channel>  Communication channel 0-31
- -p <0-3> Output power 0-3 (0=30dBm - 3=21dBm)
- -a <0-7> Air speed 0=1Kbps(default)
 -			1=2Kbps
 -			2=5Kbps
 -			3=8Kbps
 -			4=10Kbps
 -			5=15Kbps
 -			6=20Kbps
 -			7=25Kbps
- -s <0-7> uart speed
 -			0=1200bps
 -			1=2400bps
 -			2=4800bps
 -			3=9600bps
 -			4=19200bps
 -			5=38400bps
 -			6=57600bps
 -			7=115200bps

