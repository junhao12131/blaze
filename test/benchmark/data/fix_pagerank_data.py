import random

def main():
    FILENAME = 'pagerank_data.txt'
    SCALE = 5
    fin = open(FILENAME, 'r')
    lines = []
    vertices = set()
    for line in fin:
        line = line.strip()
        lines.append(line)
        line = line.split(' ')
        vertices.add(int(line[0]))
        vertices.add(int(line[1]))
    fin.close()

    nv = 1 << SCALE
    for i in range(nv):
        if not i in vertices:
            lines.append(str(random.randint(0, nv)) + ' ' + str(i))

    fout = open(FILENAME, 'w')
    fout.write('\n'.join(lines))

main()
