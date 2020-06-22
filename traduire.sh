#!/bin/bash

lupdate -no-obsolete *.pro
linguist *.ts
lrelease *.pro
