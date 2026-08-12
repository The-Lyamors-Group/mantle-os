#!/usr/bin/mantle
let project = "mantle-test"
print "Installation de ${project}"
if command.exists("true") {
    run true
}
require admin {
    run true
}
