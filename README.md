# feature-interaction-detector

## Installation

**Dependencies:**

LLVM, Clang and Python3 are main dependencies of this project. To install them you can use this command:

`sudo apt install llvm llvm-dev clang libclang-dev python-clang`

**Make:**

Make the project using `make` standard command. It will make all classes and files and output the executable `cfg` file.

**Run:**

To run the project you should prepare and input text file. Input file name must be `Result.txt` with the format below:

```
from 5470 , to 1420 , # paths 4
- shuttle_manager::ShuttleManagerNodelet::ShuttleConfirmationCb CALL shuttle_manager::ShuttleManagerNodelet::processNextShuttleRequest WRITE shuttle_manager::ShuttleManagerNodelet::current_state
- shuttle_manager::ShuttleManagerNodelet::ShuttleConfirmationCb WRITE shuttle_manager::ShuttleManagerNodelet::current_state VARINFFUNC shuttle_manager::ShuttleManagerNodelet::processNextShuttleRequest WRITE shuttle_manager::ShuttleManagerNodelet::current_state
from 4966 , to 105 , # paths 7
- route_publisher::RoutePublisherNodelet::refGPSCb WRITE route_publisher::RoutePublisherNodelet::ref_gps_flag_ VARINFFUNC route_publisher::RoutePublisherNodelet::updatePath CALL route_publisher::RoutePublisherNodelet::fillPathsFromRoutePlan WRITE route_plan
- route_publisher::RoutePublisherNodelet::refGPSCb WRITE route_publisher::RoutePublisherNodelet::first_init_path VARINFFUNC route_publisher::RoutePublisherNodelet::updatePath CALL route_publisher::RoutePublisherNodelet::fillPathsFromRoutePlan WRITE route_plan
```

After providing the input file you can run the main file to process your input and make the output using command below:

`python main.py`

The output of each line in your text file will be generated in `output/` folder in its specific group.

Structure of output folder:

```
output/
    ------ 0/
        ------ 0/
            ------ constraints.txt
        ------ 1/
            ------ constraints.txt
    ------ 1/
        ------ 0/
            ------ constraints.txt
        ------ 1/
            ------ constraints.txt
```