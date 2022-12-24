
file(REMOVE_RECURSE global)

execute_process(
  COMMAND ${CMAKE_COMMAND} -P gtags_update.cmake
  WORKING_DIRECTORY ".."
  COMMAND_ERROR_IS_FATAL ANY
  )

find_program(HTAGS htags)
message("HTAGS=${HTAGS}")

execute_process(
  COMMAND ${HTAGS} -ans --tabs 4 -F
  WORKING_DIRECTORY ".."
  COMMAND_ERROR_IS_FATAL ANY
  )

file(MAKE_DIRECTORY global)
file(RENAME ../HTML global/HTML)
