#!/bin/sh

scons build/po/ru.po && mv po/ru.po po/ru.po_ && cp build/po/ru.po po/ru.po && env LANG=ru_RU.UTF-8 poedit po/ru.po && scons msgupdate
