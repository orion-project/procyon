# Dependencies

All the commands below suppose the current directory is `deps`.

## hunspell

[hunspell](http://hunspell.github.io/) used for spell checking.

```bash
git clone https://github.com/hunspell/hunspell
cd hunspell
git checkout v1.7.0
cd ..
```

**Download dictionaries**

In the example below, the package is the full set of dictionaries for [LibreOffice](https://github.com/LibreOffice/dictionaries) (74M), and we have to extract only needed `.dic` and `.aff` files.

```bash
# Don't forget to leave hunspell directory
cd ..

curl https://codeload.github.com/LibreOffice/dictionaries/zip/libreoffice-6.3.0.4 > libreoffice-6.3.0.4.zip
unzip -j libreoffice-6.3.0.4.zip dictionaries-libreoffice-6.3.0.4/en/en_US.* -d ../bin/dicts
unzip -j libreoffice-6.3.0.4.zip dictionaries-libreoffice-6.3.0.4/ru_RU/ru_RU.* -d ../bin/dicts
```

## hoedown

[hoedown](https://github.com/hoedown/hoedown) used to render [markdown](https://en.wikipedia.org/wiki/Markdown) memos to display HTML. It included into the project as source code files so not special build operations needed, you only have to clone the repo.

```bash
git clone https://github.com/hoedown/hoedown
cd hoedown
git checkout 3.0.7
cd ..
```
