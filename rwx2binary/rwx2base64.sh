#!/bin/bash

npx browserify -t envify -p esmify main.js 2>/dev/null | npx browser-run
