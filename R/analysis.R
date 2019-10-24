
# "includes"
#
source("support_scripts/package_code_extract_lib.R")
library(propagatr) # for the actual analysis

# Functions that might be useful.
# 
analyze_package <- function(pname) {

    # get all package files
    # (this gives the file locations)
    locs <- extract_package_code(pname)

    analyze_file <- function(file_path) {
        
        # get file name (for writing output)
        file_name <- basename(file_path)
        type_of_file <- basename(dirname(file_path))
        
        propagatr::dyntrace_types({
            source(file_path)
        }, paste("./results", pname, type_of_file, "", sep="/"), file_name)
    }

    # TODO make sure packages are available

    lapply(locs, function(sub_list_of_paths) {
        lapply(sub_list_of_paths, function(actual_path) {
            analyze_file(actual_path)
        })
    })
}

# The actual script stuff
#
args <- commandArgs(trailingOnly=T)
pname <- args[1]

analyze_package(pname)
