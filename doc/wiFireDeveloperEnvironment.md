
![](../img.png)  

## Creator WiFire development environment setup
----


### Objectives
The objective of this document is to guide the developer through the setup and use of a suitable development environment for the ChipKit WiFire development board. Instructions are provided for both Windows and Linux based systems.

### Requirements
The hardware and software requirements listed below represent the most straightforward development environment. There are alternatives available but this document assumes the use of the components as listed. Further information can be found by clicking the appropriate links below.  

All the software listed below is available free of charge for both Windows and Linux.

### Hardware
* Digilent ChipKIT WiFire development board (PIC32MZ2048EFG100 processor)  
* MPLAB ICD 3 – Microchip In-Circuit Debugger ([setup information available here](doc/ID3.md)) *or* PICkit™ 3 In-Circuit Debugger ([setup information available here](doc/PICkit3.md))   
* Adaptor cable – to connect ICD to WiFire  
* USB cable – for command console  

### Software and tools
* Git for [Windows](https://git-scm.com/download/win) or [Linux](https://git-scm.com/download/linux)
* XC32 v1.42 - C cross compiler for PIC microcontrollers  
* MPLABX IDE v3.35 - Microchip PIC integrated development environment  
* Harmony v1_07_01 - integrated firmware development platform for PIC32 microcontrollers  

### Installing on Windows

### Installing Harmony v1_07_01  

**Note.** *The WiFire application is designed to work specifically with Harmony v1.07.01. Successful operation is not guaranteed with other versions.* Version 1.07.01 is available for download under the *archived downloads* tab at the following link:  

[Harmony v1_07_01 - integrated firmware development platform for PIC32 microcontrollers. ](http://www.microchip.com/mplab/mplab-harmony)  

After downloading, run the executable *harmony_vX_XXX_windows_installer.exe*. You may need administrator rights on your PC.  

During installation:  

* Use the dialogue to modify the installation directory to *C:\microchip\harmony\current*  
* Accept all other default conditions  

### Setting up the MPLAB® X Development Environment  
The development environment consists of the MPLAB X IDE and the MPLAB® XC32 C cross compiler.  

### The MPLAB® XC32 C cross compiler  

Download the compiler from the following link:  

[XC32 v1.42 - C cross compiler for PIC microcontrollers](http://www.microchip.com/mplab/compilers)  

After downloading, run the installer *xc32-vX.XX-windows-installer.exe*.  

During installation:  

* Accept the default installation folder but make a note of the path by copying from the dialogue box.  
* Select the option to 'Add xc32 to the PATH environment variable' and click ‘Next’.  

When installation completes, click 'Next' to accept the free license version (unless you have a licence activation code for the Pro version).  


### The MPLAB X Integrated Development Environment (IDE)  

Download the MPLAB X IDE from the following link:  

[MPLAB X IDE v3.35 - Microchip PIC integrated development environment](http://www.microchip.com/mplab/mplab-x-ide)  

After downloading, run the installer *MPLABX-vX.XX-windows-installer.exe*.  

During installation:

* Accept the default installation folder but make a note of the path by copying from the dialogue box.  
* Select the IDE and the IPE installation checkboxes.
* Uncheck the options to download XC32 etc. and click 'Next' to finish the installation.  



----

## Installing on Linux  

Root access is required to install the development tools in Linux.   

### Install MPLAB® Harmony Integrated Software Framework

After downloading the installer, use the command:

```
./harmony_vX_XX_linux_installer.run --mode text
```  

When prompted, change the installation directory to: *~/microchip/harmony/current*  

**Note:** the *--mode text* option runs the installer in text mode as there are problems with the graphical interface on some Linux distributions.

### Install the WiFire Harmony SDK

This will need to be extracted from *WiFireHarmonySDK_vX.X.X.zip* after downloading, using the command:

`unzip -o WiFireHarmonySDK_vX.X.X.zip -d ~/microchip/harmony`  

The paths in the project files need to be changed to Linux paths using a script provided, use the following commands to change the project files:

```
cd ~/microchip/harmony/current/third_party  
./linux_setup.sh
```

### Install the MPLAB® XC32 C compiler

Use the commands:
```
tar -xvf xc32-vX.XX-linux-installer.run.tar
sudo ./xc32-vX.XX-linux-installer.run --mode text
```

When prompted add XC32 to the *path* environment variable and select the free license.

###	Install MPLAB® X Integrated Development Environment

After downloading, use the commands:
```
tar -xvf MPLABX-vX.XX-linux-installer.tar
sudo ./MPLABX-vX.XX-linux-installer.sh
```

Reboot after installing the tools to finalise the installation process.  

---   
For further information please visit:  
* [The CreatorDev forum](https://forum.creatordev.io)  
* [CreatorDev online documentation](https://docs.creatordev.io/wifire)  

---


### Next  

[Building the WiFire application](BuildingTheWiFireApplication.md)  

### Previous

[Back to README](../README.md)  

----

----





