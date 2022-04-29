export BLUECAT_TARGET_CPU=i386
export BLUECAT_VERSION=4.100
if [ `uname | cut -c1-6` = "CYGWIN" ]
then
  curdir=`pwd`
  curdir=`cygpath -w "$curdir" | sed -e 's%\\\%/%g'`
  drive=`echo $curdir | cut -c1`
  drive=`echo $drive | tr [a-z] [A-Z]`
  rest=`echo $curdir | cut -c4-`
  export BLUECAT_PREFIX=//$drive/$rest
  export TMPDIR=/tmp
  export BASH=/bin/bash.exe
else
  export BLUECAT_PREFIX=`pwd`
fi

if [ $# -eq 0 ]; then
  unset BLUECAT_TARGET_BSP
  rm -f $BLUECAT_PREFIX/demo
  rm -f $BLUECAT_PREFIX/usr/share/bluecat
  (cd $BLUECAT_PREFIX/usr/src; ln -snf linux.default linux)
else
  BLUECAT_TARGET_BSP="$1"
  if [   ! -d $BLUECAT_PREFIX/usr/src/linux.${BLUECAT_TARGET_BSP} \
      -o ! -d $BLUECAT_PREFIX/demo.${BLUECAT_TARGET_BSP} ]; then
    echo "ERROR: BlueCat BSP '$BLUECAT_TARGET_BSP' is not installed." >&2
    unset BLUECAT_TARGET_BSP
    rm -f $BLUECAT_PREFIX/demo
    rm -f $BLUECAT_PREFIX/usr/share/bluecat
    (cd $BLUECAT_PREFIX/usr/src; ln -snf linux.default linux)
  else
    export BLUECAT_TARGET_BSP
    (cd $BLUECAT_PREFIX;         ln -snf demo.${BLUECAT_TARGET_BSP}  demo )
    (cd $BLUECAT_PREFIX/usr/src; ln -snf linux.${BLUECAT_TARGET_BSP} linux)
    (cd $BLUECAT_PREFIX/usr/share
     if [ -d bluecat.${BLUECAT_TARGET_BSP} ]; then
       ln -snf bluecat.${BLUECAT_TARGET_BSP} bluecat
     else
       rm -f bluecat
     fi
    )
  fi
fi

if [ "${USER_PS1:-X}" = "X" ]; then
export USER_PATH="$PATH"
export USER_PS1="${PS1:=$ }"
if [ `uname | cut -c1-6` = "CYGWIN" ]
then
  export BLUECAT_CWD="$BLUECAT_PREFIX"
else
  export BLUECAT_CWD=`bash -c pwd`
fi
PATH="\
$BLUECAT_PREFIX/cdt/bin:\
$BLUECAT_PREFIX/cdt/X11R6/bin:\
$BLUECAT_PREFIX/cdt/${BLUECAT_TARGET_CPU}-lynx-linux-bluecat/bin:\
$BLUECAT_PREFIX/cdt/sbin:\
$PATH"
export MANPATH=$BLUECAT_PREFIX/cdt/man:$BLUECAT_PREFIX/usr/man
fi

PS1="BlueCat:$USER_PS1"
