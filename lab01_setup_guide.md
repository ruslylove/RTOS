# Lab 1: Toolchain and Environment Setup Guide

This guide will walk you through setting up the development environment, debug probe firmware, and real-time visualization tools for the **FRDM-MCXN236** development board and **FreeRTOS** on macOS.

---

## Step 1: Development Environment Setup (Choose Option A or B)

### Option A: MCUXpresso IDE (Eclipse-based, Easiest)

1. **Install IDE**:
   - Go to [NXP MCUXpresso IDE](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE).
   - Download the installer for macOS (`.dmg`) and follow the installation wizard.
2. **Install SDK**:
   - Go to the [MCUXpresso SDK Builder](https://mcuxpresso.nxp.com/).
   - Select the `FRDM-MCXN236` board and click **Build SDK** (verify **FreeRTOS** is selected in components).
   - Drag-and-drop the downloaded SDK `.zip` file into the **Installed SDKs** panel at the bottom of the IDE workspace.

### Option B: VS Code + MCUXpresso Extension (Modern, Recommended)

1. **Install VS Code Extensions**:
   - Open VS Code, go to the Extensions Marketplace (`Cmd+Shift+X`), and search/install **MCUXpresso for VS Code** (published by NXP).
   - This automatically installs necessary helpers like *C/C++*, *CMake Tools*, and *Cortex-Debug*.
2. **Install Toolchain and Build Tools**:
   - **Method A (Automatic)**:
     - Open the newly added **MCUXpresso** tab in the sidebar.
     - Click **Install MCUXpresso Toolchain**. This opens the NXP Toolchain Manager which will download and register the verified **GCC ARM Embedded Compiler**, **CMake**, and **Ninja** for you.
   - **Method B (Manual / Homebrew)**:
     - If you prefer managing your own tools, install them via Terminal using Homebrew:
       ```bash
       # Install CMake and Ninja build tools
       brew install cmake ninja
       
       # Install Arm GNU Toolchain (Compiler)
       brew install --cask gcc-arm-embedded
       ```
     - Tell the MCUXpresso extension where the toolchain is located:
       - Open VS Code Settings (`Cmd + ,`).
       - Search for `MCUXpresso: Toolchain Path`.
       - Set it to the compiler path, typically: `/Applications/ArmGNUToolchain/13.2.rel1/arm-none-eabi` (or your custom install path).
3. **Download & Import SDK**:
   - Under the MCUXpresso tab, click **Install SDK**. Search for `FRDM-MCXN236` and download it directly.
   - Alternatively, you can build and download the `.zip` file from the [NXP SDK Builder](https://mcuxpresso.nxp.com/) and import it via the extension.
4. **Create / Import a Project**:
   - In the MCUXpresso panel, click **Import Project**.
   - Choose the imported `FRDM-MCXN236` SDK, and select a FreeRTOS example (such as `freertos_hello` or `freertos_blink`) to generate your workspace.

---

## Step 2: Install SEGGER Tools

To inspect the real-time scheduling behavior of FreeRTOS, you need the SEGGER debugging software and SystemView:

1. **J-Link Software and Documentation Pack**:
   - Go to [SEGGER J-Link Downloads](https://www.segger.com/downloads/jlink/).
   - Download and install the **J-Link Software and Documentation Pack for macOS** (`.pkg`).
   - This provides the drivers and flash utilities for macOS.

2. **SEGGER SystemView**:
   - Go to [SEGGER SystemView Downloads](https://www.segger.com/downloads/systemview/).
   - Download and install **SystemView for macOS**. This is the GUI client that streams and visualizes task switches and interrupts.

---

## Step 3: Re-flash MCU-Link Debugger to J-Link Firmware

The FRDM-MCXN236 board has an onboard **MCU-Link** debug probe (the smaller chip section on the side of the board). By default, it runs NXP's LinkServer firmware. To run SEGGER SystemView, the debugger must run **SEGGER J-Link** firmware instead.

1. **Locate the Firmware Utility**:
   - Install the standalone **MCU-Link Installer** or locate the scripts inside your MCUXpresso IDE installation directory:
     ```
     /Applications/NXP/MCU-Link_Installer/scripts
     ```
2. **Put MCU-Link into ISP (In-System Programming) Mode**:
   - Unplug the board from your computer.
   - Locate the **ISP jumper** (labeled `J4` or `JP1` / `ISP` depending on the board revision, situated next to the debugger USB port).
   - **Fit a jumper shunt** across the ISP pins.
   - Connect the board to your Mac using the USB port labeled **MCU-LINK** (or **DEBUG**). The red Link LED should flash rapidly or stay dim, indicating it is in bootloader/ISP mode.
3. **Run the Update Script**:
   - Open terminal on your Mac and navigate to the scripts folder:
     ```bash
     cd /Applications/NXP/MCU-Link_Installer/scripts
     ```
   - Execute the programming script for J-Link:
     ```bash
     ./program_jlink.sh
     ```
   - Wait for the programmer script to complete successfully.
4. **Finalize Firmware Mode**:
   - Unplug the USB cable from the board.
   - **REMOVE the ISP jumper shunt** (if left in place, the board will boot into ISP mode next time it's powered).
   - Plug the USB cable back in. The board will now enumerate as a **SEGGER J-Link** debugger.

---

## Step 4: Integrate SystemView into your FreeRTOS Project

To enable tracing inside your FreeRTOS program, you must include the SystemView source files and patch the FreeRTOS configuration:

1. **Copy SystemView Source Files**:
   - Copy the following files from your SystemView installation directory (usually found under `/Applications/SEGGER/SystemView/Src` or the download folder):
     - `SEGGER_SYSVIEW.c` & `SEGGER_SYSVIEW.h`
     - `SEGGER_SYSVIEW_ConfDefaults.h` & `SEGGER_SYSVIEW_Int.h`
     - `SEGGER_RTT.c` & `SEGGER_RTT.h`
     - `SEGGER_RTT_printf.c`
     - `Config/SEGGER_SYSVIEW_Config_FreeRTOS.c`
     - `Sample/OS/SEGGER_SYSVIEW_FreeRTOS.h`
   - **For MCUXpresso IDE (Eclipse)**: Add these files directly to your project (e.g., drag them into a folder named `SystemView`). Eclipse will automatically find and build them.
   - **For VS Code (CMake-based)**:
     - Create a folder (e.g. `source/SystemView`) in your project and paste the files.
     - Open `CMakeLists.txt` and add the new source files and include directories to your target:
       ```cmake
       target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
           source/SystemView
       )
       target_sources(${CMAKE_PROJECT_NAME} PRIVATE
           source/SystemView/SEGGER_SYSVIEW.c
           source/SystemView/SEGGER_SYSVIEW_Config_FreeRTOS.c
           source/SystemView/SEGGER_RTT.c
           source/SystemView/SEGGER_RTT_printf.c
       )
       ```

2. **Update `FreeRTOSConfig.h`**:
   - Open your project's `FreeRTOSConfig.h` and add the trace facility macros:
     ```c
     #define configUSE_TRACE_FACILITY          1
     #define configRECORD_STACK_HIGH_ADDRESS   1
     ```
   - Append the SystemView config include at the very end of `FreeRTOSConfig.h`:
     ```c
     #include "SEGGER_SYSVIEW_FreeRTOS.h"
     ```

3. **Initialize SystemView in `main()`**:
   - In your application's `main.c`, initialize SystemView before calling `vTaskStartScheduler()`:
     ```c
     #include "SEGGER_SYSVIEW.h"

     int main(void) {
         // Hardware Init (Clocks, Pins, etc.)
         BOARD_InitBootPins();
         BOARD_InitBootClocks();
         
         // Initialize SEGGER SystemView
         SEGGER_SYSVIEW_Conf();
         
         // Create your FreeRTOS tasks
         xTaskCreate(vBlinkTask, "Blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
         
         // Start the Scheduler
         vTaskStartScheduler();
         for(;;);
     }
     ```

---

## Step 5: Capture a Trace

1. **Flash & Run the project**:
   - **In MCUXpresso IDE**: Click the **Debug** button in the Quickstart Panel.
   - **In VS Code**: Click the **Build** button in the status bar, then navigate to the **Run and Debug** view (`Cmd+Shift+D`), select your launch configuration (e.g. `J-Link Debug`), and click **Start Debugging** (green play arrow).
2. Run the application.
3. Open **SEGGER SystemView** on your Mac.
4. Select **Target** > **Start Recording** (or press `F5`).
5. Choose connection parameters:
   - **Target Device**: `MCXN236`
   - **Interface**: `SWD`
   - **Speed**: `Auto` or `4000 kHz`
6. Click **OK**. You will see the real-time Gantt chart timeline of your FreeRTOS tasks executing, context switches, and interrupts!

---

## Troubleshooting: Installing Extension on Custom IDEs (like Antigravity IDE)

If you are using **antigravity-ide** (or another open-source editor that defaults to the Open VSX registry), the NXP MCUXpresso extension will not show up in the marketplace search because NXP only publishes it on the official Visual Studio Marketplace.

You can download and install it manually:

1. **Download the VSIX**:
   On macOS, download the VSIX bundle from the marketplace endpoint (note that it downloads as a gzip file and must be decompressed):
   ```bash
   curl -L "https://marketplace.visualstudio.com/_apis/public/gallery/publishers/NXPSemiconductors/vsextensions/mcuxpresso/latest/vspackage" -o mcuxpresso.vsix.gz
   gunzip mcuxpresso.vsix.gz
   ```

2. **Install to your IDE**:
   Run the CLI install command:
   ```bash
   antigravity-ide --install-extension mcuxpresso.vsix
   ```
   Or use the **Install from VSIX...** option in your editor's Extensions view menu (`...`).
