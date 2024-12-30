# Macrosilicon/UltraSemi MS2130 firmware patcher
## Disables sharpening, scaling, etc.

This tool patches the [firmware with 4K support](https://github.com/damnnfo/ms2130-firmware/blob/main/4k%20not%20tested/MS2130%204K2.bin) and applies the register changes also found in [hsdaoh](https://github.com/steve-m/hsdaoh).

You can use [ms-tools](https://github.com/BertoldVdb/ms-tools) to flash the firmware.
Make sure to backup your firmware:

    ./cli --log-level=7 read FLASH 0 --filename=backup.bin

and then flash the new firmware with:

    ./cli --log-level=7 --no-patch write-file FLASH 0 ms2130_patched_v1.bin


## Comparison

Here is a comparison, courtesy of 'Muf' on the Domesday86 Discord:

### Before, with unpatched firmware:
![unpatched](https://raw.githubusercontent.com/steve-m/ms2130_patcher/refs/heads/master/unpatched.png)

***

### After, with patched firmware:
![patched](https://raw.githubusercontent.com/steve-m/ms2130_patcher/refs/heads/master/patched.png)
