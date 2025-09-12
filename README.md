Use ProjectorSwitch with the Zoom conferencing application on Windows to move the second Zoom window to a designated monitor and back to its original position.

Download ProjectorSwitch.exe into a folder and run it. Select the target monitor from the drop-down list and click the **Toggle** button to move the Zoom window to the chosen monitor. Click the button again to move it back.

The application stores its settings in the settings.ini file in the same folder as the executable file, so make sure you have write permissions to the folder.

Application logs are stored in the **Logs** folder (within your installation folder).

**Optional command-line arguments:**

		--help | -h | /?         Show help dialog and exit.
  
		--toggle                 Toggle the Zoom window once.
  
		--no-gui                 Run headless (no window).
  
		--monitor <key|index>    Preselect monitor (Key or 1-based index).
	
 	Examples:
  
		ProjectorSwitch.exe --toggle --no-gui
  
		ProjectorSwitch.exe --monitor 2
  
		ProjectorSwitch.exe --monitor MONITOR_KEY_STRING
