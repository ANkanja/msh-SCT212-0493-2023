#!/bin/sh
# Kills itself with SIGTERM so msh should show status 128+15 = 143
kill -TERM $$
