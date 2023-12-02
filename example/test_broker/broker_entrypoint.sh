#!/bin/bash
set -e
# from: https://github.com/scholzj/docker-qpid-cpp/blob/master/docker-entrypoint.sh

# if command starts with an option, prepend qpidd
if [ "${1:0:1}" = '-' ]; then
    set -- qpidd "$@"
fi

if [ "$1" = "qpidd" ]; then
    sasl_external=0
    sasl_plain=0
    have_acl=0
    have_sasl=0
    have_paging=0
    have_auth=0
    have_config=0

    # Home dir
    if [ -z "$QPIDD_HOME" ]; then
        QPIDD_HOME="/var/lib/qpidd"
    fi

    if [ ! -d "$QPIDD_HOME" ]; then
        mkdir -p "$QPIDD_HOME"
        chown -R qpidd:qpidd "$QPIDD_HOME"
    fi

    # Data dir (and also PID dir)
    if [ -z "$QPIDD_DATA_DIR" ]; then
        QPIDD_DATA_DIR="$QPIDD_HOME/work"
    fi

    if [ ! -d "$QPIDD_DATA_DIR" ]; then
        mkdir -p "$QPIDD_DATA_DIR"
        chown -R qpidd:qpidd "$QPIDD_DATA_DIR"
    fi

    #####
    # If SASL database already exists, change the password only when it was provided from outside.
    # If it doesn't exist, create it either with password from env or with default password
    #####
    if [ -z "$QPIDD_SASL_DB"]; then
        QPIDD_SASL_DB="$QPIDD_HOME/etc/sasl/qpidd.sasldb"
    fi

    mkdir -p "$(dirname $QPIDD_SASL_DB)"

    have_auth=1

    #####
    # Create SASL config if it doesn't exist, create it
    #]####
    if [ -z "$QPIDD_SASL_CONFIG_DIR" ]; then
        QPIDD_SASL_CONFIG_DIR="$QPIDD_HOME/etc/sasl/"
    fi

    if [ ! -f "$QPIDD_SASL_CONFIG_DIR/qpidd.conf" ]; then
        if [[ $sasl_plain -eq 1 || $sasl_external -eq 1 ]]; then
            mkdir -p "$(dirname $QPIDD_SASL_CONFIG_DIR)"

            mechs="ANONYMOUS"

            if [ $sasl_plain -eq 1 ]; then
                mechs="PLAIN DIGEST-MD5 CRAM-MD5 $mechs"
            fi

            if [ $sasl_external -eq 1 ]; then
                mechs="EXTERNAL $mechs"
            fi

            cat > $QPIDD_SASL_CONFIG_DIR/qpidd.conf <<-EOS
mech_list: $mechs
pwcheck_method: auxprop
auxprop_plugin: sasldb
sasldb_path: $QPIDD_SASL_DB
sql_select: dummy select
EOS
            have_sasl=1
        fi
    fi

    #####
    # Create ACL file - if user was set and the ACL env var not, generate it.
    #####
    if [ -z "$QPIDD_ACL_FILE" ]; then
        QPIDD_ACL_FILE="$QPIDD_HOME/etc/qpidd.acl"
    fi

    cat > $QPIDD_ACL_FILE <<-EOS
acl allow anonymous@QPID all
acl deny-log all all
EOS
    have_acl=1

    #####
    # Store dir configuration
    #####
    if [ -z $QPIDD_STORE_DIR ]; then
        QPIDD_STORE_DIR="$QPIDD_HOME/store"
    fi

    #####
    # Paging dir configuration
    #####
    if [ -z $QPIDD_PAGING_DIR ]; then
        QPIDD_PAGING_DIR="$QPIDD_HOME/paging"
    fi

    mkdir -p "$QPIDD_PAGING_DIR"
    have_paging=1

    #####
    # Generate broker config file if it doesn`t exist
    #####
    if [ -z "$QPIDD_CONFIG_FILE" ]; then
        QPIDD_CONFIG_FILE="$QPIDD_HOME/etc/qpidd.conf"
    fi

    if [ "$QPIDD_CONFIG_OPTIONS" ]; then
        cat >> $QPIDD_CONFIG_FILE <<-EOS
$QPIDD_CONFIG_OPTIONS
EOS
	      have_config=1
    else
        if [ ! -f "$QPIDD_CONFIG_FILE" ]; then
            cat >> $QPIDD_CONFIG_FILE <<-EOS
data-dir=$QPIDD_DATA_DIR
pid-dir=$QPIDD_DATA_DIR
EOS

            if [ $have_sasl -eq "1" ]; then
                cat >> $QPIDD_CONFIG_FILE <<-EOS
sasl-config=$QPIDD_SASL_CONFIG_DIR
EOS
                have_config=1
            fi

            if [ $have_auth -eq "1" ]; then
                cat >> $QPIDD_CONFIG_FILE <<-EOS
auth=yes
EOS
                have_config=1
            fi

            if [ $have_acl -eq "1" ]; then
                cat >> $QPIDD_CONFIG_FILE <<-EOS
acl-file=$QPIDD_ACL_FILE
EOS
                have_config=1
            fi

            if [ $have_paging -eq "1" ]; then
                cat >> $QPIDD_CONFIG_FILE <<-EOS
paging-dir=$QPIDD_PAGING_DIR
EOS
                have_config=1
            fi

            cat >> $QPIDD_CONFIG_FILE <<-EOS
port=5672
EOS
            have_config=1
        else
            have_config=1
        fi
    fi

    if [ $have_config -eq "1" ]; then
        set -- "$@" "--config" "$QPIDD_CONFIG_FILE"
    fi

    #chown -R qpidd:qpidd "$QPIDD_HOME"
fi

# else default to run whatever the user wanted like "bash"
exec "$@"
