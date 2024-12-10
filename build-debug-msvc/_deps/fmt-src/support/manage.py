#!/usr/bin/env python3

"""Manage site and releases.

Usage:
  manage.py release [<branch>]
  manage.py site

For the release command $FMT_TOKEN should contain a GitHub personal access token
obtained from https://github.com/settings/tokens.
"""

from __future__ import print_function
import datetime, docopt, errno, fileinput, json, os
import re, requests, shutil, sys, tempfile
from contextlib import contextmanager
from distutils.version import LooseVersion
from subprocess import check_call


class Git:
    def __init__(self, dir):
        self.dir = dir

    def call(self, method, args, **kwargs):
        return check_call(['git', method] + list(args), **kwargs)

    def add(self, *args):
        return self.call('add', args, cwd=self.dir)

    def checkout(self, *args):
        return self.call('checkout', args, cwd=self.dir)

    def clean(self, *args):
        return self.call('clean', args, cwd=self.dir)

    def clone(self, *args):
        return self.call('clone', list(args) + [self.dir])

    def commit(self, *args):
        return self.call('commit', args, cwd=self.dir)

    def pull(self, *args):
        return self.call('pull', args, cwd=self.dir)

    def push(self, *args):
        return self.call('push', args, cwd=self.dir)

    def reset(self, *args):
        return self.call('reset', args, cwd=self.dir)

    def update(self, *args):
        clone = not os.path.exists(self.dir)
        if clone:
            self.clone(*args)
        return clone


def clean_checkout(repo, branch):
    repo.clean('-f', '-d')
    repo.reset('--hard')
    repo.checkout(branch)


class Runner:
    def __init__(self, cwd):
        self.cwd = cwd

    def __call__(self, *args, **kwargs):
        kwargs['cwd'] = kwargs.get('cwd', self.cwd)
        check_call(args, **kwargs)


def create_build_env():
    """Create a build environment."""
    class Env:
        pass
    env = Env()

    # Import the documentation build module.
    env.fmt_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sys.path.insert(0, os.path.join(env.fmt_dir, 'doc'))
    import build

    env.build_dir = 'build'
    env.versions = build.versions

    # Virtualenv and repos are cached to speed up builds.
    build.create_build_env(os.path.join(env.build_dir, 'virtualenv'))

    env.fmt_repo = Git(os.path.join(env.build_dir, 'fmt'))
    return env


@contextmanager
def rewrite(filename):
    class Buffer:
        pass
    buffer = Buffer()
    if not os.path.exists(filename):
        buffer.data = ''
        yield buffer
        return
    with open(filename) as f:
        buffer.data = f.read()
    yield buffer
    with open(filename, 'w') as f:
        f.write(buffer.data)


fmt_repo_url = 'git@github.com:fmtlib/fmt'


def update_site(env):
    env.fmt_repo.update(fmt_repo_url)

    doc_repo = Git(os.path.join(env.build_dir, 'fmtlib.github.io'))
    doc_repo.update('git@github.com:fmtlib/fmtlib.github.io')

    for version in env.versions:
        clean_checkout(env.fmt_repo, version)
        target_doc_dir = os.path.join(env.fmt_repo.dir, 'doc')
        # Remove the old theme.
        for entry in os.listdir(target_doc_dir):
            path = os.path.join(target_doc_dir, entry)
            if os.path.isdir(path):
                shutil.rmtree(path)
        # Copy the new theme.
        for entry in ['_static', '_templates', 'basic-bootstrap', 'bootstrap',
                      'conf.py', 'fmt.less']:
            src = os.path.join(env.fmt_dir, 'doc', entry)
            dst = os.path.join(target_doc_dir, entry)
            copy = shutil.copytree if os.path.isdir(src) else shutil.copyfile
            copy(src, dst)
        # Rename index to contents.
        contents = os.path.join(target_doc_dir, 'contents.rst')
        if not os.path.exists(contents):
            os.rename(os.path.join(target_doc_dir, 'index.rst'), contents)
        # Fix issues in reference.rst/api.rst.
        for filename in ['reference.rst', 'api.rst', 'index.rst']:
            pattern = re.compile('doxygenfunction.. (bin|oct|hexu|hex)$', re.M)
            with rewrite(os.path.join(target_doc_dir, filename)) as b:
                b.data = b.data.replace('std::ostream &', 'std::ostream&')
                b.data = re.sub(pattern, r'doxygenfunction:: \1(int)', b.data)
                b.data = b.data.replace('std::FILE*', 'std::FILE *')
                b.data = b.data.replace('unsigned int', 'unsigned')
                #b.data = b.data.replace('operator""_', 'operator"" _')
                b.data = b.data.replace(
                    'format_to_n(OutputIt, size_t, string_view, Args&&',
                    'format_to_n(OutputIt, size_t, const S&, const Args&')
                b.data = b.data.replace(
                    'format_to_n(OutputIt, std::size_t, string_view, Args&&',
                    'format_to_n(OutputIt, std::size_t, const S&, const Args&')
                if version == ('3.0.2'):
                    b.data = b.data.replace(
                        'fprintf(std::ostream&', 'fprintf(std::ostream &')
                if version == ('5.3.0'):
                    b.data = b.data.replace(
                        'format_to(OutputIt, const S&, const Args&...)',
                        'format_to(OutputIt, const S &, const Args &...)')
                if version.startswith('5.') or version.startswith('6.'):
                    b.data = b.data.replace(', size_t', ', std::size_t')
                if version.startswith('7.'):
                    b.data = b.data.replace(', std::size_t', ', size_t')
                    b.data = b.data.replace('join(It, It', 'join(It, Sentinel')
                if version.startswith('7.1.'):
                    b.data = b.data.replace(', std::size_t', ', size_t')
                    b.data = b.data.replace('join(It, It', 'join(It, Sentinel')
                    b.data = b.data.replace(
                        'fmt::format_to(OutputIt, const S&, Args&&...)',
                        'fmt::format_to(OutputIt, const S&, Args&&...) -> ' +
                        'typename std::enable_if<enable, OutputIt>::type')
                b.data = b.data.replace('aa long', 'a long')
                b.data = b.data.replace('serveral', 'several')
                if version.startswith('6.2.'):
                    b.data = b.data.replace(
                        'vformat(const S&, basic_format_args<' +
                        'buffer_context<Char>>)',
                        'vformat(const S&, basic_format_args<' +
                        'buffer_context<type_identity_t<Char>>>)')
        # Fix a broken link in index.rst.
        index = os.path.join(target_doc_dir, 'index.rst')
        with rewrite(index) as b:
            b.data = b.data.replace(
                'doc/latest/index.html#format-string-syntax', 'syntax.html')
        # Fix issues in syntax.rst.
        index = os.path.join(target_doc_dir, 'syntax.rst')
        with rewrite(index) as b:
            b.data = b.data.replace(
                '..productionlist:: sf\n', '.. productionlist:: sf\n ')
            b.data = b.data.replace('Examples:\n', 'Examples::\n')
        # Build the docs.
        html_dir = os.path.join(env.build_dir, 'html')
        if os.path.exists(html_dir):
            shutil.rmtree(html_dir)
        include_dir = env.fmt_repo.dir
        if LooseVersion(version) >= LooseVersion('5.0.0'):
            include_dir = os.path.join(include_dir, 'include', 'fmt')
        elif LooseVersion(version) >= LooseVersion('3.0.0'):
            include_dir = os.path.join(include_dir, 'fmt')
        import build
        build.build_docs(version, doc_dir=target_doc_dir,
                         include_dir=include_dir, work_dir=env.build_dir)
        shutil.rmtree(os.path.join(html_dir, '.doctrees'))
        # Create symlinks for older versions.
        for link, target in {'index': 'contents', 'api': 'reference'}.items():
            link = os.path.join(html_dir, link) + '.html'
            target += '.html'
            if os.path.exists(os.path.join(html_dir, target)) and \
               not os.path.exists(link):
                os.symlink(target, link)
        # Copy docs to the website.
        version_doc_dir = os.path.join(doc_repo.dir, version)
        try:
            shutil.rmtree(version_doc_dir)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise
        shutil.move(html_dir, version_doc_dir)


def release(args):
    env = create_build_env()
    fmt_repo = env.fmt_repo

    branch = args.get('<branch>')
    if branch is None:
        branch = 'master'
    if not fmt_repo.update('-b', branch, fmt_repo_url):
        clean_checkout(fmt_repo, branch)

    # Convert changelog from RST to GitHub-flavored Markdown and get the
    # version.
    changelog = 'ChangeLog.rst'
    changelog_path = os.path.join(fmt_repo.dir, changelog)
    import rst2md
    changes, version = rst2md.convert(changelog_path)
    cmakelists = 'CMakeLists.txt'
    for line in fileinput.input(os.path.join(fmt_repo.dir, cmakelists),
                                inplace=True):
        prefix = 'set(FMT_VERSION '
        if line.startswith(prefix):
            line = prefix + version + ')\n'
        sys.stdout.write(line)

    # Update the version in the changelog.
    title_len = 0
    for line in fileinput.input(changelog_path, inplace=True):
        if line.startswith(version + ' - TBD'):
            line = version + ' - ' + datetime.date.today().isoformat()
            title_len = len(line)
            line += '\n'
        elif title_len:
            line = '-' * title_len + '\n'
            title_len = 0
        sys.stdout.write(line)

    # Add the version to the build script.
    script = os.path.join('doc', 'build.py')
    script_path = os.path.join(fmt_repo.dir, script)
    for line in fileinput.input(script_path, inplace=True):
      m = re.match(r'( *versions = )\[(.+)\]', line)
      if m:
        line = '{}[{}, \'{}\']\n'.format(m.group(1), m.group(2), version)
      sys.stdout.write(line)

    fmt_repo.checkout('-B', 'release')
    fmt_repo.add(changelog, cmakelists, script)
    fmt_repo.commit('-m', 'Update version')

    # Build the docs and package.
    run = Runner(fmt_repo.dir)
    run('cmake', '.')
    run('make', 'doc', 'package_source')
    update_site(env)

    # Create a release on GitHub.
    fmt_repo.push('origin', 'release')
    auth_headers = {'Authorization': 'token ' + os.getenv('FMT_TOKEN')}
    r = requests.post('https://api.github.com/repos/fmtlib/fmt/releases',
                      headers=auth_headers,
                      data=json.dumps({'tag_name': version,
                                       'target_commitish': 'release',
                                       'body': changes, 'draft': True}))
    if r.status_code != 201:
        raise Exception('Failed to create a release ' + str(r))
    id = r.json()['id']
    uploads_url = 'https://uploads.github.com/repos/fmtlib/fmt/releases'
    package = 'fmt-{}.zip'.format(version)
    r = requests.post(
        '{}/{}/assets?name={}'.format(uploads_url, id, package),
        headers={'Content-Type': 'application/zip'} | auth_headers,
        data=open('build/fmt/' + package, 'rb'))
    if r.status_code != 201:
        raise Exception('Failed to upload an asset ' + str(r))


if __name__ == '__main__':
    args = docopt.docopt(__doc__)
    if args.get('release'):
        release(args)
    elif args.get('site'):
        update_site(create_build_env())
