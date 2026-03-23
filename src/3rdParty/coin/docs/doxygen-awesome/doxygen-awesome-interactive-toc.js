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

class DoxygenAwesomeInteractiveToc {
    static topOffset = 38
    static hideMobileMenu = true
    static headers = []

    static init() {
        window.addEventListener("load", () => {
            let toc = document.querySelector(".contents > .toc")
            if(toc) {
                toc.classList.add("interactive")
                if(!DoxygenAwesomeInteractiveToc.hideMobileMenu) {
                    toc.classList.add("open")
                }
                document.querySelector(".contents > .toc > h3")?.addEventListener("click", () => {
                    if(toc.classList.contains("open")) {
                        toc.classList.remove("open")
                    } else {
                        toc.classList.add("open")
                    }
                })

                document.querySelectorAll(".contents > .toc > ul a").forEach((node) => {
                    let id = node.getAttribute("href").substring(1)
                    DoxygenAwesomeInteractiveToc.headers.push({
                        node: node,
                        headerNode: document.getElementById(id)
                    })

                    document.getElementById("doc-content")?.addEventListener("scroll", () => {
                        DoxygenAwesomeInteractiveToc.update()
                    })
                })
                DoxygenAwesomeInteractiveToc.update()
            }
        })
    }

    static update() {
        let active = DoxygenAwesomeInteractiveToc.headers[0]?.node
        DoxygenAwesomeInteractiveToc.headers.forEach((header) => {
            let position = header.headerNode.getBoundingClientRect().top
            header.node.classList.remove("active")
            header.node.classList.remove("aboveActive")
            if(position < DoxygenAwesomeInteractiveToc.topOffset) {
                active = header.node
                active?.classList.add("aboveActive")
            }
        })
        active?.classList.add("active")
        active?.classList.remove("aboveActive")
    }
}