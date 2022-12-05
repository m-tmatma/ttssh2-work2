
set(SETUP_URL "https://cygwin.com/setup-x86_64.exe")
set(SETUP_HASH_SHA256 "edd0a64dc65087ffe453ca94b267169b39458a983b29ac31320fcaa983d0f97e")

file(DOWNLOAD
  ${SETUP_URL}
  "c:/cygwin64/setup-x86_64.exe"
  EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
  SHOW_PROGRESS
  )

execute_process(
  COMMAND c:/cygwin64/setup-x86_64.exe --quiet-mode --upgrade-also
  )
