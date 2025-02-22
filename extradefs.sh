#!/bin/bash

# include source files in extradefs.depend
# add comment "extradefs(scope::enum)" to enums

# param1: dest_path

current_dir=$(dirname $0)
dest_path=$1

[[ $OSTYPE =~ msys ]] && grep_span=-Ezo
[[ $OSTYPE =~ [lL]inux ]] && grep_span=-Pzoa

function handle_enums() {
    local header=${1##*/}
    local macro=${header%.*}
    macro=EXTRA_${macro^^}

    printf "\n\n\n#ifdef $macro\n#ifndef ${macro}_H\n#define ${macro}_H\n\n#include \"$header\"" >> $dest_path

    for param in ${@:2}; do
        local enum=${param##*::}
        local scope=${param%::*}
        [[ $enum = $scope ]] && scope="" || scope=$scope::

        local enum_decl="enum\s+(class\s+)?$enum"
        local code=$(grep $grep_span "$enum_decl\s*{[^}]+};" $1 | tr -d "\r\0" | sed "s#%#%%#g")
        local scope_self=""
        [[ -n $(printf "$code" | grep -Eo "^enum\s+class\s+") ]] && scope_self=$enum::

        printf "\n\n" >> $dest_path

        printf "$code" |
            sed -r "s#^$enum_decl#const extra_map<$scope$enum> EXTRA_${enum^^} =#;
                    s#^\s*##;
                    s#\s*\$##;
                    s#^//#@@#;
                    s#^([A-Za-z0-9_]+)\s*(,?)\s*//\s*(.*)#    {$scope$scope_self\1, {EXTRA_STR(\"\3\")\}\}\2#;
                    s#\s*//\s*#\"), EXTRA_STR(\"#g;
                    s#^@@#    //#" >> $dest_path
    done

    printf "\n\n#endif\n#endif" >> $dest_path
}

printf "\
#ifndef EXTRADEFS_H\n\
#define EXTRADEFS_H\n\n\
#include <string>\n\
#include <vector>\n\
#include <map>\n\n\
#define EXTRA_STRING_T std::wstring\n\
#define EXTRA_STR(s) L##s\n\n\
class extra_vector : public std::vector<EXTRA_STRING_T> {\n\
public:\n\
    using std::vector<EXTRA_STRING_T>::vector;\n\n\
    inline EXTRA_STRING_T string(size_t pos) const {\n\
        EXTRA_STRING_T str;\n\
        try { str = this->at(pos); } catch (...) {}\n\
        return str;\n\
    }\n\n\
    inline int integer(size_t pos) const {\n\
        int num = 0;\n\
        try { num = std::stoi(string(pos)); } catch (...) {}\n\
        return num;\n\
    }\n\
};\n\n\
template<class key_t>\n\
class extra_map : public std::map<key_t, extra_vector> {\n\
public:\n\
    using std::map<key_t, extra_vector>::map;\n\n\
    inline EXTRA_STRING_T string(key_t key, size_t pos) const {\n\
        auto iter = this->find(key);\n\
        return this->end() != iter ? iter->second.string(pos) : EXTRA_STRING_T();\n\
    }\n\n\
    inline int integer(key_t key, size_t pos) const {\n\
        auto iter = this->find(key);\n\
        return this->end() != iter ? iter->second.integer(pos) : 0;\n\
    }\n\
};\n\n\
#endif" > $dest_path

for line in $(cat $current_dir/extradefs.depend | tr -d "\r\0"); do
    source_path=$current_dir/$line
    handle_enums $source_path $(sed -nr "s#.*extradefs\((.+)\).*#\1#p" $source_path)
done
