# HunterCrossing
Fundamentals Of Game Engine Design - Final Project
* [Demo Video](https://youtu.be/w0Wt_YP0mmc)

# Installation

* Successfully install the AfterBurner Game Engine.
  * [Download](https://www.dropbox.com/s/yhaosl3cshlvshv/repo_distro.7z?dl=0)
  * [Tutorial](https://youtu.be/hPGZf2dHSG0)
* Create a new module, rename it to Hunter Crossing (using renameModule.py).
* Drop the contents of the github repo HunterCrossing/HunterCrossing inside of the local module.
* Add the github LIBs content to your local aburn/usr/lib64 folder.
* Run script inside of local HunterCrossing (RUN CMAKE GUI.bat) to create the local HunterCrossing module.
  * Make sure these lines are present after generating with cmake:
    * GDAL_INCLUDE_DIR | Your local location.
    * GDAL_LIBRARY | Your local location.
    * FREETYPE_INCLUDE_DIRS | Your local location.
    * FREETYPE_LIBRARY | Your local location.
* Add the DLLs to your local modules/HunterCrossing/cwin64.
* Download [mm folder](https://www.dropbox.com/scl/fi/slluh4cyugwo9b7oq6qfp/mm.zip?rlkey=nt227wnny0ieu0s68op9hb8ku&st=5pz02mb1&dl=0), add it to your local modules/HunterCrossing as "mm".

* Lastly, run the solution or exe and enjoy!
