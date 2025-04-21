function git_repo_pack_with_submodules() {
    declare path= dir_name= source_name=""

    path="$GIT_ROOT"


    git_check_path "$path" || return

    if [ -z "$dir_name" ]; then
        dir_name="$(git_url_path_name "$path")" || return
    fi

    if [ -z "$dir_name" ]; then
        log_error "Could not derive dir_name from remote URL and path."
        return 1
    fi

    if [ -z "$source_name" ]; then
        dirty_flag=

        path_status="$(git_status "$path")" || return
        if [ -n "$path_status" ]; then
            dirty_flag="-dirty"
        fi

        source_name="${dir_name}-${GIT_HEAD_SHORT:-$GIT_BRANCH}${dirty_flag}.tar.gz"
    fi

    if [ -z "$OUTDIR" ]; then
        log_debug "OUTDIR is not set. No action taken."
        output "$source_name"
        return
    fi

    log_info "$souerce_name   $dirname"

    (
      cd "$GIT_ROOT" || exit 1
      git ls-files --recurse-submodules \
       | tar caf "$OUTDIR/$source_name" \
         --xform "s:^:$dir_name/:" \
         --verbatim-files-from -T -
    )

    log_info "Wrote: $OUTDIR/$source_name"

    output "$source_name"
}
