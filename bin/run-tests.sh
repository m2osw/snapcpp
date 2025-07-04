#!/bin/sh -e
#
# Run all the tests

export HOME=`cd && pwd`

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
CONTRIBS="`find contrib -maxdepth 1 ! -path 'contrib' -type d` | sort"
SNAPWEBSITES="`find snapwebsites -maxdepth 1 ! -path 'snapwebsites' -type d` | sort"

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

START_DATE=`date -u`

CONVERT="BUILD/Debug/contrib/snaplogger/tools/convert-ansi --output-style-tag"
if ! test -x "${CONVERT}"
then
    CONVERT=cat
fi

IS_SNAPWEBSITES_DEFINED=false
if getent passwd snapwebsites >/dev/null \
    && getent group snapwebsites >/dev/null \
    && getent group adm | sed -e 's/.*://' -e 's/,/ /' | grep "\<${USER}\>" > /dev/null
then
    IS_SNAPWEBSITES_DEFINED=true
fi

if test "${TYPE}" = "html"
then
    HTMLDIR="${TOPDIR}/BUILD/html"
    rm -rf "${HTMLDIR}"
    mkdir -p "${HTMLDIR}"
    HTML="${HTMLDIR}/index.html"
    echo "<html>" > "${HTML}"
    echo "<head>" >> "${HTML}"
    echo "<title>Test Results</title>" >> "${HTML}"
    echo "<style>" >> "${HTML}"
    echo "body{font-family:sans-serif;}" >> "${HTML}"
    echo "table{border-collapse:collapse;}" >> "${HTML}"
    echo "th{background-color:#f0f0f0;}" >> "${HTML}"
    echo "td,th{border:1px solid black;padding:5px;}" >> "${HTML}"
    echo "tr.no-test{background-color:#eeeeee;}" >> "${HTML}"
    echo "tr.success{background-color:#eeffee;}" >> "${HTML}"
    echo "tr.error{background-color:#ffeeee;}" >> "${HTML}"
    echo "</style>" >> "${HTML}"
    echo "</head>" >> "${HTML}"
    echo "<body>" >> "${HTML}"
    if test "${IS_SNAPWEBSITES_DEFINED}" != "true"
    then
        echo "<p style=\"color:red;font-size:135%\">WARNING: snapwebsites user/group not properly setup. See bin/install-for-tests.sh for details.</p>" >> "${HTML}"
    fi
    echo "<table>" >> "${HTML}"
    echo "<tr><th>Project</th><th>Test Results</th></tr>" >> "${HTML}"
fi

if test "${SYNC}" = "true"
then
    # sync the repo
    #
    if test "${TYPE}" = "html"
    then
        SYNC_OUTPUT="${HTMLDIR}/sync.html"
        echo "<html>" > "${SYNC_OUTPUT}"
        echo "<head>" >> "${SYNC_OUTPUT}"
        echo "<title>Source Synchronization</title>" >> "${SYNC_OUTPUT}"
        echo "</head>" >> "${SYNC_OUTPUT}"
        echo "</head>" >> "${SYNC_OUTPUT}"
        echo "<body>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "<h1>Source Synchronization</h1>" >> "${SYNC_OUTPUT}"
        echo "<pre>" >> "${SYNC_OUTPUT}"
        bin/check-status.sh --latest 2>&1 | ${CONVERT} >> "${SYNC_OUTPUT}" 2>&1
        echo "</pre>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "</body>" >> "${SYNC_OUTPUT}"
        echo "</html>" >> "${SYNC_OUTPUT}"
    else
        bin/check-status.sh --latest
    fi

    # and after sync-ing we need to recompile
    #
    if test "${TYPE}" = "html"
    then
        COMPILE_OUTPUT="${HTMLDIR}/compile.html"
        echo "<html>" > "${COMPILE_OUTPUT}"
        echo "<head>" >> "${COMPILE_OUTPUT}"
        echo "<title>Source Synchronization</title>" >> "${COMPILE_OUTPUT}"
        echo "</head>" >> "${COMPILE_OUTPUT}"
        echo "</head>" >> "${COMPILE_OUTPUT}"
        echo "<body>" >> "${COMPILE_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${COMPILE_OUTPUT}"
        echo "<h1>Snap! C++ Compile</h1>" >> "${COMPILE_OUTPUT}"
        echo "<pre>" >> "${COMPILE_OUTPUT}"

        # We want to detect whether make returns an error or not
        # so we save the output in an intermediate file
        #
        RAW_COMPILE_OUTPUT="${HTMLDIR}/raw-compile.txt"
        if ! make -C BUILD/Debug > "${RAW_COMPILE_OUTPUT}" 2>&1
        then
            echo "<p style=\"color:red;font-size:135%\">ERROR: the <a href=\"compile.html\" rel=\"nofollow\">compile step</a> failed.</p>" >> "${HTML}"
        fi
        ${CONVERT} "${RAW_COMPILE_OUTPUT}" >> "${COMPILE_OUTPUT}" 2>&1
        rm -f "${RAW_COMPILE_OUTPUT}"

        echo "</pre>" >> "${COMPILE_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${COMPILE_OUTPUT}"
        echo "</body>" >> "${COMPILE_OUTPUT}"
        echo "</html>" >> "${COMPILE_OUTPUT}"
    else
        make -C BUILD/Debug
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
                    echo "<tr class=\"error\"><td>${d}</td><td>error -- <a href=\"${ERROR}\" rel=\"nofollow\">see output</a></td></tr>" >> "${HTML}"

                    echo "<html>" > "${HTMLDIR}/${ERROR}"
                    echo "<head>" >> "${HTMLDIR}/${ERROR}"
                    echo "<title>Output of ${d} tests</title>" >> "${HTMLDIR}/${ERROR}"
                    echo "<meta name=\"robots\" content=\"noindex\"/>" >> "${HTMLDIR}/${ERROR}"
                    echo "</head>" >> "${HTMLDIR}/${ERROR}"
                    echo "<body>" >> "${HTMLDIR}/${ERROR}"
                    echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${HTMLDIR}/${ERROR}"
                    echo "<pre>" >> "${HTMLDIR}/${ERROR}"
                    ${CONVERT} "${OUTPUT}" >> "${HTMLDIR}/${ERROR}"
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
                    echo "<tr class=\"success\"><td>${d}</td><td>success</td></tr>" >> "${HTML}"
                fi
            fi
        else
            if test "${TYPE}" = "html"
            then
                echo "<tr class=\"no-test\"><td>${d}</td><td>no tests found -- skipped</td></tr>" >> "${HTML}"
            else
                echo "--- SKIPPING: ${d} ---"
            fi
        fi
    )
done

END_DATE=`date -u`

if test "${TYPE}" = "html"
then
    echo "</table>" >> "${HTML}"
    if test "${SYNC}" = "true"
    then
        echo "<p><a href=\"sync.html\">Sync output</a></p>" >> "${HTML}"
        echo "<p><a href=\"compile.html\">Snap! C++ Compile</a></p>" >> "${HTML}"
    fi
    if test ${FAILURES} -ne 0
    then
        PLURAL="s"
        if test ${FAILURES} -eq 1
        then
            PLURAL=""
        fi
        echo "<p>Got ${FAILURES} error${PLURAL}.</p>" >> "${HTML}"
    fi
    echo "<p>Process started on ${START_DATE} and ending on ${END_DATE}.</p>" >> "${HTML}"
    echo "<br/>" >> "${HTML}"
    echo '<div style="text-align:center;"><a href="https://snapwebsites.org/">Snap C++</a> | <a href="..">List of projects</a></div>' >> "${HTML}"
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
