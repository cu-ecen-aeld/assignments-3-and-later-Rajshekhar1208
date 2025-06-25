#!/bin/sh

echo "Running test script"

cd /home  # Force working directory



chmod +x finder.sh finder-test.sh writer  # Force executable permissions


rc=$?
if [ ${rc} -eq 0 ]; then
    echo "Completed with success!!"
else
    echo "Completed with failure, failed with rc=${rc}"
fi
echo "finder-app execution complete, dropping to terminal"
/bin/sh
