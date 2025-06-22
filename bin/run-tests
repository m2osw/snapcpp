#!/bin/sh -e
#
# Run all the tests

TOPDIR="`pwd`"
while true
do
    BASE="`basename ${TOPDIR}`"
    if test "${BASE}" = "snapcpp"
    then
        break
    fi
    TOPDIR="`dirname ${TOPDIR}`"
done

cd "${TOPDIR}"

BUILD="${TOPDIR}/BUILD/Debug"
CONTRIBS="`find contrib -maxdepth 1 ! -path 'contrib' -type d`"
SNAPWEBSITES="`find snapwebsites -maxdepth 1 ! -path 'snapwebsites' -type d`"

TYPE=console
while test -n "${1}"
do
    case "${1}" in
    "-h"|"--help")
        echo "Usage: `basename ${0}` [--opts]"
        echo "where --opts is one or more of:"
        echo "       --console           output results in plain text"
        echo "  -h | --help              print out this help screen"
        echo "       --html              output results in HTML"
        echo
        echo "TOPDIR = ${TOPDIR}"
        echo "CONTRIBS = ${CONTRIBS}"
        echo "SNAPWEBSITES = ${CONTRIBS}"
        exit 1
        ;;

    "--console")
        TYPE=console
        shift
        ;;

    "--html")
        TYPE=html
        shift
        ;;

    *)
        echo "error: unknown command line option \"${1}\"."
        exit 1
        ;;

    esac
done

if test "${TYPE}" = "html"
then
    HTMLDIR="${TOPDIR}/BUILD/html"
    mkdir -p "${HTMLDIR}"
    HTML="${HTMLDIR}/index.html"
    echo "<html>" > "${HTML}"
    echo "<head>" >> "${HTML}"
    echo "<title>Test Results</title>" >> "${HTML}"
    echo "</head>" >> "${HTML}"
    echo "<body>" >> "${HTML}"
    echo "<table>" >> "${HTML}"
    echo "<tr><th>Project</th><th>Test Results</th></tr>" >> "${HTML}"
fi

SUCCESS=true
for d in ${CONTRIBS} snapwebsites
do
    (
        cd "${d}"
        if test -d tests
        then
            if test "${TYPE}" = "console"
            then
                echo "--- RUNNING TESTS OF: ${d} ---"
            fi
            OUTPUTDIR="${BUILD}/${d}"
            mkdir -p "${OUTPUTDIR}"
            OUTPUT="${OUTPUTDIR}/test-full-output.txt"
            if ! ./mk -t >"${OUTPUT}" 2>&1
            then
                if test "${TYPE}" = "html"
                then
                    ERROR="`basename ${d}`.html"
                    echo "<tr><td>${d}</td><td>error -- <a href=\"${ERROR}\">see output</a></td></tr>" >> "${HTML}"
                    echo "<html>" > "${HTMLDIR}/${ERROR}"
                    echo "<head>" >> "${HTMLDIR}/${ERROR}"
                    echo "<title>Output of ${d} tests</title>" >> "${HTMLDIR}/${ERROR}"
                    echo "</head>" >> "${HTMLDIR}/${ERROR}"
                    echo "<body>" >> "${HTMLDIR}/${ERROR}"
                    echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${HTMLDIR}/${ERROR}"
                    echo "<pre>" >> "${HTMLDIR}/${ERROR}"
                    cat "${OUTPUT}" >> "${HTMLDIR}/${ERROR}"
                    echo "</pre>" >> "${HTMLDIR}/${ERROR}"
                    echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${HTMLDIR}/${ERROR}"
                    echo "</body>" >> "${HTMLDIR}/${ERROR}"
                    echo "</html>" >> "${HTMLDIR}/${ERROR}"
                else
                    echo "   an error occurred, see output here: ${OUTPUT}"
                    echo
                fi
                SUCCESS=false
            else
                if test "${TYPE}" = "html"
                then
                    echo "<tr><td>${d}</td><td>success</td></tr>" >> "${HTML}"
                fi
            fi
        else
            if test "${TYPE}" = "html"
            then
                echo "<tr><td>${d}</td><td>no tests found -- skipped</td></tr>" >> "${HTML}"
            else
                echo "--- SKIPPING: ${d} ---"
            fi
        fi
    )
done

if test "${TYPE}" = "html"
then
    echo "</body>" >> "${HTML}"
    echo "</html>" >> "${HTML}"
fi

$SUCCESS

# vim: ts=4 sw=4 et
