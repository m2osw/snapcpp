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

PUBLISH=false
PUBLISHDIR="/mnt/lcov/tests"
SYNC=false
TYPE=console
while test -n "${1}"
do
    case "${1}" in
    "--console")
        TYPE=console
        shift
        ;;

    "-h"|"--help")
        echo "Usage: `basename ${0}` [--opts]"
        echo "where --opts is one or more of:"
        echo "       --console           output results in plain text"
        echo "  -h | --help              print out this help screen"
        echo "       --html              output results in HTML"
        echo "       --info              display some information (variables)"
        echo "  -p | --publish [dir]     send HTML output to lcov website"
        echo "       --sync              synchronize the git environment"
        exit 1
        ;;

    "--html")
        TYPE=html
        shift
        ;;

    "--info")
        echo "TOPDIR = ${TOPDIR}"
        echo "CONTRIBS = ${CONTRIBS}"
        echo "SNAPWEBSITES = ${CONTRIBS}"
        exit 1
        ;;

    "-p"|"--publish")
        PUBLISH=true
        shift
        if test -n "${1}"
        then
            case "${1}" in
            "-"*)
                # assume this is another option
                ;;

            *)
                PUBLISHDIR="${1}"
                shift
                ;;

            esac
        fi
        ;;

    "--sync")
        SYNC=true
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
    rm -rf "${HTMLDIR}"
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

if test "${SYNC}" = "true"
then
    if test "${TYPE}" = "html"
    then
        SYNC_OUTPUT="${HTMLDIR}/sync.html"
        echo "<html>" > "${SYNC_OUTPUT}"
        echo "<head>" >> "${SYNC_OUTPUT}"
        echo "</head>" >> "${SYNC_OUTPUT}"
        echo "<body>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "<pre>" >> "${SYNC_OUTPUT}"
        bin/check-status.sh -l >> "${SYNC_OUTPUT}"
        echo "</pre>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "</body>" >> "${SYNC_OUTPUT}"
        echo "</html>" >> "${SYNC_OUTPUT}"
    else
        bin/check-status.sh -l
    fi
fi

FAILURES=0
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
                FAILURES=`expr ${FAILURES} + 1`
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
    echo "</table>" >> "${HTML}"
    if test "${SYNC}" = "true"
    then
        echo "<p><a href=\"sync.html\">Sync output</a></p>" >> "${HTML}"
    fi
    if test ${FAILURES} -ne 0
    then
        PLURAL="s"
        if test ${FAILURES} -ne 0
        then
            PLURAL=""
        fi
        echo "<p>Got ${FAILURES} error${PLURAL}.</p>" >> "${HTML}"
    fi
    echo "</body>" >> "${HTML}"
    echo "</html>" >> "${HTML}"

    # publish the results, that means deleting now working tests
    # and then replacing with newly failing ones (or none if it's
    # all in working order which is probably going to be rare)
    #
    if test "${PUBLISH}" = "true" -a -d "${PUBLISHDIR}"
    then
        rm -f "${PUBLISHDIR}/"*
        cp -r "${HTMLDIR}/"* "${PUBLISHDIR}/."
    fi
fi

if test ${FAILURES} -eq 0
then
    exit 0
fi

echo "error: ${FAILURES} projects failed."

# vim: ts=4 sw=4 et
