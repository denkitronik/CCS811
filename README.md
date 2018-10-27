# Linux Raspbian (Raspberry PI) Shared Library for AMS AG CCS811 - Ultra-Low Power Digital Gas Sensor for Monitoring Indoor Air Quality

This is an easy to use shared library for CO2 and TVOC sensor CCS811

## Getting Started


### Prerequisites

WiringPi library installed

### Installing
ENABLING I2C

1. Start the Raspberry Pi config utility: 
sudo raspi-config

2. Select "Interfacing Options"

3. Select "I2C"

4. In the message: Would you like the ARM I2C interface to be enabled? Select "yes" and then select on "Ok"

5. Select "Finish"


INSTALL AND UPDATE WIRING PI

Enter to the Raspberry PI shell (directly with a screen and a keyboard or using a SSH client in your PC) follow the next steps (see http://wiringpi.com/download-and-install/):

1. Check you have a updated version of wiringPi that works with your Raspberry PI model:

gpio -v


2. If you need or want to get the last version of wiringPi uninstall the current version of wiringPi:

sudo apt-get purge wiringpi

hash -r


3. Let's update to the last software repository and install "git":

sudo apt-get update

sudo apt-get install git-core


4. Go to your home directory and get wiringPi:

cd

git clone git://git.drogon.net/wiringPi


5. Enter to the wiringPi directory and check you have the last version:

cd ~/wiringPi

git pull origin


6. Build and install the library, just typing the following: 

./build


INSTALL LIBCCS811

1. Let's check if you have updated the software repository and install "git":

sudo apt-get update

sudo apt-get install git-core


2. Go to your home directory and get "libccs811":

cd

git clone git://github.com/denkitronik/CCS811


3. Enter to the libccs811 directory and check you have the last version:

cd ~/CCS811

git pull origin


4. Build and install the library, just typing the following: 

./make


## Running the tests

1. Connect the CCS811 in the 3.3v power supply pins of your Raspberry PI.
2. Connect the CCS811 SCLK pin to the pin 5 of your Raspberry PI.
3. Connect the CCS811 SDA pin to the pin 3 of your Raspberry PI.
4. Connect the CCS811 ADDRESS pin in the 3.3v power supply pin of your Raspberry PI.
5. Connect the CCS811 INT pin in the pin 13 (wiringPi 2) of your Raspberry PI.
6. Connect the CCS811 RESET pin in the pin 15 (wiringPi 3) of your Raspberry PI.


See the following image:
![alt text](https://github.com/denkitronik/CCS811/blob/master/pinout.png)

For more information visit: [pinout.xyz](https://pinout.xyz/pinout/i2c#)

## Example
An example is included in the examples folder.

You can run it in this simple way:

```bash
cd examples
gcc example.c -lccs811 -lwiringPi -o example
./example
```

## Built With

* [Netbeans IDE](https://netbeans.apache.org/download/nb90/nb90.html)

## Contributing

Everyone is invited to contribute
When contributing to this repository, please first discuss the change you wish to make via issue, email, or any other method with me before making a change.

## Release History
    1.0.0
        CHANGE: Initial commit

## Versioning
I am using [SemVer](http://semver.org/) for versioning. 

## Authors

* **Alvaro Salazar** - *Initial work* - [alvaro-salazar](https://github.com/alvaro-salazar)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE.md) file for details.
