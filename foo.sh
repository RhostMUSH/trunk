#!/bin/bash
    version_regex='^([0-9]+)\.([0-9]+)(\+.+)*$'
    current_tag=$(git describe --tags --abbrev=0 --candidates=1)
    current_tag=$(echo ${current_tag} | tr -d 'v')
    if [[ ${current_tag} =~ ${version_regex} ]]; then
        current_major="${BASH_REMATCH[1]}"
        current_minor="${BASH_REMATCH[2]}"
        current_patch=$(git grep PATCH_VERSION $(git ls-remote . 'refs/remotes/*/HEAD' |cut -f 1) | grep -v pre-commit |grep -v pre-push | grep ':hdrs/patchlevel.h:' | awk '{print $3}' |tr -d '"')
    else
        echo " "
        echo "ERROR: 'git describe --tags --candidates=1' did not output a valid version" >&2
        echo "        string for the current repo. Unable to use patchlevel.template." >&2
        echo "        This requires manual fixing of the repository." >&2
        exit 1
    fi
    pfile_major=$(grep "#define MAJOR_VERSION " $(pwd)/Server/hdrs/patchlevel.template | awk '{print $3}' |tr -d '"')
    pfile_minor=$(grep "#define MINOR_VERSION " $(pwd)/Server/hdrs/patchlevel.template | awk '{print $3}' |tr -d '"')
    pfile_patch=$(grep "#define PATCH_VERSION " $(pwd)/Server/hdrs/patchlevel.template | awk '{print $3}' |tr -d '"')
    pfile_ext=$(grep "#define VERSION_EXT " $(pwd)/Server/hdrs/patchlevel.template | awk '{print $3}' |tr -d '"')
    is_larger=0
    major_changed=0
    minor_changed=0
    patch_changed=0
    for i in major minor patch
    do
        tempvar=pfile_${i}
        tempvar2=current_${i}
        if [[ "${!tempvar}" -lt "${!tempvar2}" ]];then
            if [[ "$(( $major_changed + $minor_changed + $patch_changed ))" -eq "0" ]];then
                echo "ERROR: The new version number can not be lower than the current one!" >&2
                exit 1
            fi
        elif [[ "${!tempvar}" -gt "${!tempvar2}" ]];then
           eval ${i}_changed=1
        fi
    done
    if [[ "$(( ${major_changed} + ${minor_changed} + ${patch_changed} ))" -eq "0" ]];then
        echo "ERROR: The new version number must be different from the old one!" >&2
        exit 1
    fi
