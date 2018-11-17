import os
import re
import sys

def file_list(dirname):
    files = []
    for name in os.listdir(dirname):
        full_path = os.path.join(dirname, name)
        if os.path.isfile(full_path):
            files.append(full_path)
        elif os.path.isdir(full_path):
            files += file_list(full_path)
    return files

def read_file(path):
    guard = re.compile(r'(#ifndef|#endif)')
    include_dep = re.compile(r'#include "(.+?)"')
    include = re.compile(r'#include <(.+?)>')

    out = ''
    dep = []
    includes = []
    with open(path) as f:
        for line in f.readlines():
            if len(guard.findall(line)) > 0:
                continue

            include_name = include_dep.findall(line)
            if len(include_name) > 0:
                dep.append(include_name[0])
                continue

            include_name = include.findall(line)
            if len(include_name) > 0:
                includes.append(include_name[0])
                continue

            out += line

    return out, dep, includes

def in_endswith(name, files):
    for f in files:
        if f.endswith(name):
            return f
    return None

def order_dep(deps, files, done):
    out = ''
    includes = []
    for dep in deps:
        if in_endswith(dep, done) is None:
            path = in_endswith(dep, files)

            f, d, i = read_file(path)
            dep_out, dep_include = order_dep(d, files, done)
            
            out += dep_out + f
            includes += dep_include + i
            done.append(path)

    return out, includes

def merge(outfile, dirname):
    files = file_list(dirname)

    out = ''
    done = []
    includes = []
    for full_path in files:
        if full_path not in done:
            data, dep, include = read_file(full_path)
            dep_out, dep_include = order_dep(dep, files, done)

            out += dep_out + data
            includes += dep_include + include
            done.append(full_path)

    includes = os.linesep.join('#include <{}>'.format(x) for x in includes) + '\n'

    with open(outfile, 'w') as f:
        f.write(includes)
        f.write(out)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        outfile = sys.argv[1]
    else:
        outfile = './concurrency.hpp'
    
    if len(sys.argv) > 2:
        dirname = sys.argv[2]
    else:
        dirname = './concurrency/impl'

    merge(outfile, dirname)
