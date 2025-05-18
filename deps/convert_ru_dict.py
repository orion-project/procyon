with open('ru_RU.aff', 'r', encoding='koi8-r') as f:
    s = f.read()
s = s.replace('SET KOI8-R', 'SET UTF-8')
with open('../bin/dicts/ru_RU.aff', 'w', encoding='utf8') as f:
    f.write(s)

with open('ru_RU.dic', 'r', encoding='koi8-r') as f:
    s = f.read()
with open('../bin/dicts/ru_RU.dic', 'w', encoding='utf8') as f:
    f.write(s)
