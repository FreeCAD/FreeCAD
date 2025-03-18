import os
import json
import glob


def append_to_file(filename, text):
    with open(filename, "a") as f:
        f.write(text)


def main():
    # Retrieve environment variables
    github_env = os.environ.get("GITHUB_ENV")
    github_step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    artifacts_dir = os.environ.get("artifactsDownloadDir")

    if not github_env or not github_step_summary or not artifacts_dir:
        print(
            "One or more required environment variables (GITHUB_ENV, GITHUB_STEP_SUMMARY, artifactsDownloadDir) are missing."
        )
        return

    # Write initial marker to GITHUB_ENV
    append_to_file(github_env, "usedArtifacts<<EOD\n")

    # Open or create the report markdown file
    report_md = "report.md"
    with open(report_md, "w") as report_file:
        pass  # ensure the file exists and is empty

    # Load JSON data from file named "data"
    with open("data", "r") as f:
        data = json.load(f)

    # Iterate over each step in the JSON data (assumes keys_unsorted is the list of keys)
    # Here we assume data is a dict with step names as keys.
    for step in data:
        print(f"Processing step {step}")

        step_info = data[step]
        result = step_info.get("result")

        # Determine the icon based on the result
        icon = ":heavy_check_mark:"
        if result == "failure":
            icon = ":x:"
        elif result == "cancelled":
            icon = ":no_entry_sign:"
        elif result == "skipped":
            icon = ":white_check_mark:"

        # Append the header line to report.md
        append_to_file(report_md, f"### {icon} {step} step\n")

        # Handle different result states
        if result == "skipped":
            msg = "Step was skipped, no report was generated\n"
            print(msg.strip())
            append_to_file(report_md, msg)
            continue
        elif result == "cancelled":
            msg = "Step was cancelled when executing, report may be incomplete\n"
            print(msg.strip())
            append_to_file(report_md, msg)

        # Retrieve the report file name from outputs
        report_filename = step_info.get("outputs", {}).get("reportFile")
        if report_filename:
            msg = f"Report for step {step} is {report_filename}\n"
            print(msg.strip())
            append_to_file(github_env, report_filename + "\n")

            # Search for files in artifactsDownloadDir matching report_filename recursively
            search_pattern = os.path.join(artifacts_dir, "**", report_filename)
            matching_files = glob.glob(search_pattern, recursive=True)

            if len(matching_files) == 1:
                found_file = matching_files[0]
                with open(found_file, "r") as rf:
                    file_contents = rf.read()
                append_to_file(report_md, file_contents + "\n")
            else:
                msg = f"No or several files found for report {report_filename}, not printing\n"
                print(msg.strip())
                append_to_file(report_md, msg)
                msg = "Below files found :\n"
                print(msg.strip())
                append_to_file(report_md, msg)
                for file in matching_files:
                    msg = f"{file}\n"
                    print(msg.strip())
                    append_to_file(report_md, msg)
        else:
            msg = f"Report file was not set by step {step}\n"
            print(msg.strip())
            append_to_file(report_md, msg)

        # Add an empty line to separate steps
        append_to_file(report_md, "\n")

    # Write the ending marker to GITHUB_ENV
    append_to_file(github_env, "EOD\n")

    # Append the report.md contents to GITHUB_STEP_SUMMARY
    with open(report_md, "r") as report_file:
        report_content = report_file.read()
    append_to_file(github_step_summary, report_content)


if __name__ == "__main__":
    main()
