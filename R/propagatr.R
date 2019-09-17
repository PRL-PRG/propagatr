create_dyntracer <- function(output_dirpath,
                             verbose = FALSE,
                             truncate = TRUE,
                             binary = FALSE,
                             compression_level = 0) {

    compression_level <- as.integer(compression_level)

    .Call(C_create_dyntracer,
          output_dirpath,
          verbose,
          truncate,
          binary,
          compression_level)
}


destroy_dyntracer <- function(dyntracer) {
    invisible(.Call(C_destroy_dyntracer, dyntracer))
}


dyntrace_promises <- function(expr,
                              output_dirpath,
                              verbose = FALSE,
                              truncate = TRUE,
                              binary = FALSE,
                              compression_level = 0) {

    write(as.character(Sys.time()), file.path(output_dirpath, "BEGIN"))

    compression_level <- as.integer(compression_level)

    dyntracer <- create_dyntracer(output_dirpath,
                                  verbose,
                                  truncate,
                                  binary,
                                  compression_level)

    result <- dyntrace(dyntracer, expr)

    destroy_dyntracer(dyntracer)

    write(as.character(Sys.time()), file.path(output_dirpath, "FINISH"))

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
