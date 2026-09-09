# ELF x86-64-ABI psABI

Download latest PDF: https://gitlab.com/x86-psABIs/x86-64-ABI/-/jobs/artifacts/master/raw/x86-64-ABI/abi.pdf?job=build

## x86-64-ABI/ directory
These are the ELF x86-64 psABI LaTeX sources including
full history from the former subversion repository at svn.x86-64.org,
which is gone now.  For change please create pull requests after
discussing them at the x86-64-abi@googlegroups.com mailing list (whose
archives are at https://groups.google.com/forum/#!forum/x86-64-abi).

If you want to build locally, switch to the
[x86-64-ABI](./x86-64-ABI) directory.  Then run

    % make pdf

which will give you abi.pdf once [pdflatex](https://www.latex-project.org/get/)
is installed.

Pull requests that are accepted will automatically build a new pdf.
You can find the latest by selecting CI/CD from
the [Gitlab/Jobs](https://gitlab.com/x86-psABIs/x86-64-ABI/-/jobs) navigation
bar.  Click the download button for the document.  You can see the button
highlighted in red in the below image. ![download](/images/ci_screenshot.png)



## abitest/ directory
An old testsuite of some ABI traits, very old and incomplete meanwhile,
just here for its history.
