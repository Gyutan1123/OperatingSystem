fat = "f0 ff ff 03 70 01 ff 6f 00 07 80 00 09 a0 00 0b c0 00 0d e0 00 0f 00 01 11 20 01 13 40 01 15 60 01 ff 8f 01 19 a0 01 1b c0 01 1d e0 01 1f 00 02 21 20 02 23 40 02 25 60 02 27 80 02 29 a0 02 2b c0 02 2d f0 ff 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
table = fat.split(" ")
print(table)

result = []

for i in range(len(table)//3):
    a,b,c = table[3*i],table[3*i+1],table[3*i+2]
    y,x = int((c+b+a)[:3],16), int((c+b+a)[3:],16)
    
    
    result.append(x)
    result.append(y)
    
print(result)

file_chain = []

start = int(input("start:"))
file_chain.append(start)
now = start
while (now != 0xfff):
    now = result[now]
    file_chain.append(now)
    print(now)
print(file_chain)

for n in file_chain:
    n += 31
    c = n // 36
    h = (n - 36 * c) // 18
    s = n - 36 * c - 18 * h + 1

    print("cylinder", c, "head", h, "sector", s)