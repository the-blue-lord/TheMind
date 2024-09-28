CPP_FILE=$1
BINARY_FILE=$2

LIBRARY_FOLDER=$3
INCLUDE_FOLDER=$4

echo "[COMPILER] Starting the compilation..."
g++ $CPP_FILE -o $BINARY_FILE -I $INCLUDE_FOLDER -L $LIBRARY_FOLDER -lsfml-network -lsfml-system -Wl,-rpath=$LIBRARY_FOLDER
echo "[COMPILER] Binary compiled"