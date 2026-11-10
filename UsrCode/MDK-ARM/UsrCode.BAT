SET PATH=D:\Keil\ARM\ARMCC\Bin;D:\Cubemx\;C:\Program Files (x86)\Common Files\Oracle\Java\javapath;D:\VMware\VMware Workstation\bin\;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;D:\Matlab\runtime\win64;D:\Matlab\bin;C:\Program Files\MATLAB\MATLAB Runtime\v910\runtime\win64;C:\Program Files\dotnet\;D:\Git\cmd;D:\Python\Python310\Scripts\;D:\Python\Python310\;C:\Users\hp-pc\AppData\Local\Microsoft\WindowsApps;D:\Vscode\Microsoft VS Code\bin;D:\Vscode\Microsoft VS Code\bin\copilot-shim;C:\ProgramData\hp-pc\GitHubDesktop\bin;C:\Users\hp-pc\AppData\Local\GitHubDesktop\bin;D:\LichuangEDA\lceda-pro;D:\Cubemx\.\jre\\bin
SET CPU_TYPE=STM32F103C8
SET CPU_VENDOR=STMicroelectronics
SET UV2_TARGET=UsrCode
SET CPU_CLOCK=0x007A1200
"D:\Keil\ARM\ARMCC\Bin\ArmAsm" --Via "usrcode\startup_stm32f103xb._ia"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\main.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\gpio.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\usart.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_it.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_msp.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_gpio_ex.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_tim.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_tim_ex.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_uart.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_rcc.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_rcc_ex.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_gpio.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_dma.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_cortex.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_pwr.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_flash.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_flash_ex.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\stm32f1xx_hal_exti.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmCC" --Via "usrcode\system_stm32f1xx.__i"
"D:\Keil\ARM\ARMCC\Bin\ArmLink" --Via "UsrCode\UsrCode.lnp"
"D:\Keil\ARM\ARMCC\Bin\fromelf.exe" "UsrCode\UsrCode.axf" --i32combined --output "UsrCode\UsrCode.hex"
