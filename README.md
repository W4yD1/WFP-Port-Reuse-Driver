# WFP Port Reuse / Redirect Driver

This is a sample Windows Kernel Driver that uses the Windows Filtering Platform (WFP) to demonstrate network traffic redirection.

## Overview

The primary purpose of this driver is to intercept outbound TCP/IP v4 connections and modify their destination port. Specifically, it performs the following action:

- **Redirects all outbound TCP traffic destined for port `80` to port `8080`.**

This serves as a foundational example for more complex network filtering and manipulation tasks, such as implementing port reuse policies, transparent proxying, or network monitoring.

## How it Works

The driver uses the Windows Filtering Platform (WFP) to insert itself into the network stack.

1.  **DriverEntry**:
    -   Initializes the driver object.
    -   Opens a handle to the WFP filter engine (`FwpmEngineOpen0`).
    -   Registers a WFP "callout" (`FwpsCalloutRegister1`). A callout is a set of kernel functions that WFP calls to process network traffic.
    -   Adds a WFP "filter" (`FwpmFilterAdd0`) that tells the filter engine to send specific traffic to our callout. In this case, it targets the `FWPM_LAYER_ALE_AUTH_CONNECT_V4` layer, which handles outbound IPv4 connection requests.

2.  **ClassifyFn Callback (`PortReuseCalloutClassify`)**:
    -   This is the core function where traffic is inspected and modified.
    -   It's triggered for every outbound TCP connection attempt.
    -   It checks if the destination port is `80`.
    -   If it is, the function sets the `FWPS_RIGHT_ACTION_WRITE` flag, allowing it to modify the connection data.
    -   It then changes the `remotePort` field in the connection metadata to `8080`.
    -   The connection is then permitted to proceed with the modified destination port.

3.  **DriverUnload**:
    -   This function cleans up all registered WFP objects.
    -   It removes the filter (`FwpmFilterDeleteById0`).
    -   It unregisters the callout (`FwpsCalloutUnregisterById0`).
    -   It closes the handle to the filter engine (`FwpmEngineClose0`).
    -   This ensures the driver can be safely stopped and unloaded from the system.

## How to Build

You need the **Windows Driver Kit (WDK)** and **Visual Studio** installed.

### Using Visual Studio

1.  Create a new project in Visual Studio from the "Kernel Mode Driver, Empty (WDM)" template.
2.  Name the project `WFP-Port-Reuse-Driver`.
3.  Add all the `.c` and `.h` files from this repository to the project.
4.  In the project properties, under **Linker -> Input**, add `Fwpuclnt.lib` to the "Additional Dependencies". This is required for WFP functions.
5.  Build the project for the desired architecture (e.g., x64).

### Using the WDK Command Line

1.  Open the WDK build environment command prompt.
2.  Navigate to the directory containing the source files.
3.  Run the `build` command. The compiled driver (`.sys` file) and installer (`.inf` file) will be in a subdirectory like `objchk_amd64/amd64`.

## How to Install and Test

> **Warning:** Installing kernel drivers can cause system instability. Always do this in a test environment or virtual machine. You must **disable Secure Boot** and **enable Test Signing** mode to install this unsigned driver.

### 1. Enable Test Signing

Open an Administrator Command Prompt and run:

```cmd
bcdedit /set testsigning on
```

**Reboot your computer** for this to take effect.

### 2. Install the Driver

1.  Copy the driver files (`WFP-Port-Reuse-Driver.sys` and `WFP-Port-Reuse-Driver.inf`) to a folder on your test machine.
2.  Right-click on `WFP-Port-Reuse-Driver.inf` and select **Install**.

Alternatively, use the command line (as Administrator):

```cmd
pnputil /add-driver WFP-Port-Reuse-Driver.inf /install
```

### 3. Start the Driver

In an Administrator Command Prompt:

```cmd
sc start WFP-Port-Reuse-Driver
```

You can use a tool like `DebugView` from Sysinternals to view the `DbgPrint` messages from the driver.

### 4. Test the Redirection

1.  Start a local server that listens on port `8080`. A simple Python web server works well:
    ```sh
    python -m http.server 8080
    ```
2.  Open a web browser and navigate to `http://localhost` (which is port 80).
3.  If the driver is working, your request to port 80 will be silently redirected to port 8080, and you should see the content served by your Python server. You will also see log messages in `DebugView` indicating a redirection occurred.

### 5. Stop and Uninstall the Driver

To stop the driver:

```cmd
sc stop WFP-Port-Reuse-Driver
```

To uninstall, use `pnputil` or find the driver in Device Manager (under "System devices" or "Non-Plug and Play Drivers") and uninstall it.

After you are done testing, you can turn off test signing mode:

```cmd
bcdedit /set testsigning off
```

And reboot again.
