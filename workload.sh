#!/bin/sh

exec_name="workload"

if [ -x "/usr/bin/gmake" ]; then
    exec_make="/usr/bin/gmake"
elif [ -x "/bin/gmake" ]; then
    exec_make="/bin/gmake"
elif [ -x "/usr/local/bin/gmake" ]; then
    exec_make="/usr/local/bin/gmake"
elif [ -x "/usr/pkg/bin/gmake" ]; then
    exec_make="/usr/pkg/bin/gmake"
else
    export exec_make="make"
fi

path_build_profile=`${exec_make} path_build_profile`
if [ -d "${path_build_profile}" ]; then
host_os=`${exec_make} host_os`
path_build_name=`${exec_make} path_build_name`
target_build_profile=`${exec_make} target_build_profile`
else
path_build_profile=`${exec_make} DEBUG=y path_build_profile`
host_os=`${exec_make} DEBUG=y host_os`
path_build_name=`${exec_make} DEBUG=y path_build_name`
target_build_profile=`${exec_make} DEBUG=y target_build_profile`
fi

append_ldpath()
{
    if [ -z "${1}" ]; then
        echo "invalid argument (${0})"
    else
        if [ -d "${1}" ]; then
            if [ "${host_os}" = "darwin" ]; then
                export DYLD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${1}
            else
                export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${1}
            fi
        fi
    fi
}

append_ldpath_head()
{
    if [ -z "${1}" ]; then
        echo "invalid argument (${0})"
    else
        if [ -d "${1}" ]; then
            if [ "${host_os}" = "darwin" ]; then
                export DYLD_LIBRARY_PATH=${1}:${LD_LIBRARY_PATH}
            else
                export LD_LIBRARY_PATH=${1}:${LD_LIBRARY_PATH}
            fi
        fi
    fi
}

#append_ldpath_head "../../pgl/${path_build_name}/${target_build_profile}"
#append_ldpath_head "../../sdk/mzapi/${path_build_name}/${target_build_profile}"
#append_ldpath_head "../../thirdparty/${path_build_name}/${target_build_profile}/local/usr/lib"
append_ldpath_head ${path_build_profile}

if [ ! -x "${path_build_profile}/${exec_name}" ]; then
    echo "please make ! (MAKE=${exec_make})"
    exit 1
fi

#exec ${path_build_profile}/${exec_name} $*
${path_build_profile}/${exec_name} $*
    
# End of workload.sh
