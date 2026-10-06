## Compiling and Running for Web

1. **Install Emscripten:** 
   Follow the [official installation guide](https://emscripten.org). Note that you will need **Python** installed on your system first.

2. **Install CMake:** 
   Make sure you have CMake installed and added to your system's PATH.

3. **Install CMake Tools for Visual Studio Code:** 
   Search for the extension inside the IDE. If you can't install it directly from VS Code, go to the [Open VSX Registry](https://open-vsx.org), download the extension manually, and install it.

4. **Launch VS Code within the Emscripten Environment:**
   Go to your project's directory and run VS Code inside the activated Emscripten environment. Depending on your platform, execute one of the following commands in your terminal:
   
   * **Windows:**
     ```bash
     emsdk\emsdk_env.bat
     code .
     ```
   * **Linux / macOS:**
     ```bash
     source emsdk/emsdk_env.sh
     code .
     ```
   
   If everything is set up correctly, CMake will automatically download all dependencies and configure the project. After configuration is complete, you can build the project by pressing `CTRL+SHIFT+B` and selecting **"build"**.

5. **Run a Local Web Server & Setup Debugging:** 
   Once the project is compiled, you will need a simple HTTP server to run it. 
   * It is highly recommended to use the **"Live Preview"** extension for VS Code (also available on the [Open VSX Registry](https://open-vsx.org)).
   * If you need to debug web builds, install the **WebAssembly DWARF Debugging Extension**. This will allow you to set breakpoints directly inside VS Code.

6. **Copy Resources:** 
   Copy your `res` directory to your output folder (the directory where your `.wasm` file is located).

7. **Open in Browser:** 
   Open the project via your browser. Navigate to the address where **Live Preview** is running (and don't forget to click **"Go Live"** to start the server). 
   
   You will need to manually append the path to your output folder and `game.html` to the base URL. For example, it should look like:
   `http://192.168.0.1/out/build/emscripten-wasm/Debug/game.html`
