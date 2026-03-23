/**

Doxygen Awesome
https://github.com/jothepro/doxygen-awesome-css

MIT License

Copyright (c) 2022 - 2023 jothepro

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

class DoxygenAwesomeParagraphLink {
    // Icon from https://fonts.google.com/icons
    // Licensed under the Apache 2.0 license:
    // https://www.apache.org/licenses/LICENSE-2.0.html
    static icon = `<svg xmlns="http://www.w3.org/2000/svg" height="20px" viewBox="0 0 24 24" width="20px"><path d="M0 0h24v24H0V0z" fill="none"/><path d="M17 7h-4v2h4c1.65 0 3 1.35 3 3s-1.35 3-3 3h-4v2h4c2.76 0 5-2.24 5-5s-2.24-5-5-5zm-6 8H7c-1.65 0-3-1.35-3-3s1.35-3 3-3h4V7H7c-2.76 0-5 2.24-5 5s2.24 5 5 5h4v-2zm-3-4h8v2H8z"/></svg>`
    static title = "Permanent Link"
    static init() {
        $(function() {
            $(document).ready(function() {
                document.querySelectorAll(".contents a.anchor[id], .contents .groupheader > a[id]").forEach((node) => {
                    let anchorlink = document.createElement("a")
                    anchorlink.setAttribute("href", `#${node.getAttribute("id")}`)
                    anchorlink.setAttribute("title", DoxygenAwesomeParagraphLink.title)
                    anchorlink.classList.add("anchorlink")
                    node.classList.add("anchor")
                    anchorlink.innerHTML = DoxygenAwesomeParagraphLink.icon
                    node.parentElement.appendChild(anchorlink)
                })
            })
        })
    }
}
