#!/bin/bash

# include source files in extradefs.depend
# add comment "extradefs(scope::enum)" to enums

# param1: dest_path

current_dir=$(dirname $0)
dest_path=$1

[[ $OSTYPE =~ msys ]] && grep_span=-Ezo
[[ $OSTYPE =~ [lL]inux ]] && grep_span=-Pzoa

function extra_enums() {
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
            sed -r "s#^$enum_decl#const extra_enums<$scope$enum> EXTRA_${enum^^} =#;
                    s#^\s*##;s#\s*\$##;
                    s#^([A-Za-z0-9_]+)\s*(,?)\s*//\s*(.*)#    \{ $scope$scope_self\1, \{ EXTRA_STR(\"\3\") \} \}\2#;
                    s#\s*//\s*#\"), EXTRA_STR(\"#g" >> $dest_path
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
template<class key_t>\n\
class extra_enums : public std::map<key_t, std::vector<EXTRA_STRING_T>> {\n\
public:\n\
    using std::map<key_t, std::vector<EXTRA_STRING_T>>::map;\n\n\
    inline EXTRA_STRING_T text(key_t key, size_t pos) const {\n\
        auto iter = this->find(key);\n\
        return this->end() == iter || iter->second.size() <= pos ? EXTRA_STRING_T() : iter->second[pos];\n\
    }\n\n\
    inline int number(key_t key, size_t pos) const {\n\
        int num = 0;\n\
        try { num = std::stoi(text(key, pos)); } catch (...) {}\n\
        return num;\n\
    }\n\
};\n\n\
#endif" > $dest_path

for line in $(cat $current_dir/extradefs.depend | tr -d "\r\0"); do
    header_path=$current_dir/$line
    extra_enums $header_path $(sed -nr "s#.*extradefs\((.+)\).*#\1#p" $header_path)
done
