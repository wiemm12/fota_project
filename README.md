## 🎓 Related Certification

This project was developed as part of my learning journey in **Firmware Over-The-Air (FOTA)** and **STM32 bootloader development**.

Through this training and project, I learned how a firmware update process works from the moment a new firmware image is generated until it is safely written into the microcontroller flash memory.

### What I learned

* How an **STM32 bootloader** works and how it differs from the main application.
* How the MCU decides whether to stay in the bootloader or jump to the user application.
* How to organize and manage **Flash memory** for the bootloader and application.
* How to erase and program STM32 Flash memory safely.
* How firmware can be transferred through a communication interface such as **UART**.
* How to divide a firmware binary into packets and send it progressively to the target MCU.
* How to implement bootloader commands such as:

  * Get MCU information
  * Get Chip ID
  * Check Read Protection level
  * Erase Flash sectors
  * Write data to Flash
  * Jump to the application
* How to use **CRC** to verify the integrity of received firmware data.
* How the **Vector Table**, stack pointer and reset handler are used when jumping from the bootloader to the application.
* How a firmware binary file can be downloaded from a remote source and transferred to the STM32.
* How FOTA connects different parts of an embedded system: **cloud storage, communication, bootloader, Flash memory and the final application**.

This project helped me understand that FOTA is not only about downloading a new firmware file. It requires coordination between the **communication layer, bootloader logic, memory management, firmware validation and application startup**.

![Udemy FOTA Certificate](udemy-fota-certificate.jpg)
