#!/usr/bin/mantle
let result = "boot-script"
if command.exists("true") {
    run true
}
print "${result}"
exit 0
