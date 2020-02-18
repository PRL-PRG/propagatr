create_dyntracer <- function(output_dirpath,
                             package_under_analysis = "test",
                             analyzed_file_name = "",
                             verbose = FALSE,
                             truncate = TRUE,
                             binary = FALSE,
                             compression_level = 0) {

    compression_level <- as.integer(compression_level)

    .Call(C_create_dyntracer,
          output_dirpath,
          package_under_analysis,
          analyzed_file_name,
          verbose,
          truncate,
          binary,
          compression_level)
}


destroy_dyntracer <- function(dyntracer) {
    invisible(.Call(C_destroy_dyntracer, dyntracer))
}

# expr: program to trace
# output_dir: where to put the data files
dyntrace_types <- function( expr,
                            package_under_analysis = "test",
                            output_dirpath = "./results",
                            analyzed_file_name = "test",
                            verbose = FALSE,
                            truncate = TRUE,
                            binary = FALSE,
                            compression_level = 0,
                            debug = F) {

    # if (debug)
    #     write(as.character(Sys.time()), file.path(output_dirpath, "BEGIN"))

    compression_level <- as.integer(compression_level)

    dyntracer <- create_dyntracer(output_dirpath,
                                  package_under_analysis,
                                  analyzed_file_name,
                                  verbose,
                                  truncate,
                                  binary,
                                  compression_level)

    result <- dyntrace(dyntracer, expr)

    destroy_dyntracer(dyntracer)

    # if (debug)
    #     write(as.character(Sys.time()), file.path(output_dirpath, "FINISH"))

    result
}

write_data_table <- function(data_table,
                             filepath,
                             truncate = FALSE,
                             binary = FALSE,
                             compression_level = 0) {

    compression_level <- as.integer(compression_level)

    invisible(.Call(C_write_data_table,
                    data_table,
                    filepath,
                    truncate,
                    binary,
                    compression_level))
}


read_data_table <- function(filepath_without_ext,
                            binary = FALSE,
                            compression_level = 0) {

    binary = as.logical(binary)

    compression_level <- as.integer(compression_level)

    ext <- data_table_extension(binary, compression_level)

    filepath <- paste0(filepath_without_ext, ".", ext)

    if (!binary & compression_level == 0) {
        read.table(filepath,
                   header = TRUE,
                   sep = "\x1f",
                   comment.char = "",
                   stringsAsFactors = FALSE)
    }
    else {
        .Call(C_read_data_table,
              filepath,
              binary,
              compression_level)
    }
}


data_table_extension <- function(binary = FALSE,
                                 compression_level = 0) {

    ext <- if (binary) "bin" else "csv"

    if (compression_level == 0) ext else paste0(ext, ".zst")
}
