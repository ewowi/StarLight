Import("env")
import shutil
import glob
import os

cmd_ex = shutil.which("node")
# Check if Node.js is not installed
if cmd_ex is None:
    print('\x1b[0;31;43m' + 'Node.js is not installed or missing from PATH changes to html css js will not be processed' + '\x1b[0m')
else:
    # Install the necessary node packages for the pre-build asset bundling script
    print('\x1b[6;33;42m' + 'Install the node packages' + '\x1b[0m')

    # npm ci performs a clean install of all existing dependencies, if not installed
    if not os.path.isdir('node_modules'):
        env.Execute("npm ci")

    # if html_ui does not exist, create it (and others)
    if not os.path.exists('src/html_ui.h'):
        env.Execute("npm run build")

    latest_source = max(glob.glob('data/*.*'), key=os.path.getmtime) #any file in data folder
    latest_export = max(glob.glob('src/html_*.h'), key=os.path.getmtime)

    # if any files in data newer then html_*.h, recreate it (and others)
    if os.path.getmtime(latest_source) > os.path.getmtime(latest_export):
        print ('  updated file(s) in /data -> npm run build')
        env.Execute("npm run build")

    # Call the bundling script
    # exitCode = env.Execute("node tools/cdata.js")

    # If it failed, abort the build
    # if (exitCode):
    #   Exit(exitCode)

