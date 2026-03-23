/**

Doxygen Awesome
https://github.com/jothepro/doxygen-awesome-css

MIT License

Copyright (c) 2023 jothepro

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

class DoxygenAwesomeTabs {

    static init() {
        window.addEventListener("load", () => {
            document.querySelectorAll(".tabbed:not(:empty)").forEach((tabbed, tabbedIndex) => {
                let tabLinkList = []           
                tabbed.querySelectorAll(":scope > ul > li").forEach((tab, tabIndex) => {
                    tab.id = "tab_" + tabbedIndex + "_" + tabIndex
                    let header = tab.querySelector(".tab-title")
                    let tabLink = document.createElement("button")
                    tabLink.classList.add("tab-button")
                    tabLink.appendChild(header)
                    header.title = header.textContent
                    tabLink.addEventListener("click", () => {
                        tabbed.querySelectorAll(":scope > ul > li").forEach((tab) => {
                            tab.classList.remove("selected")
                        })
                        tabLinkList.forEach((tabLink) => {
                            tabLink.classList.remove("active")
                        })
                        tab.classList.add("selected")
                        tabLink.classList.add("active")
                    })
                    tabLinkList.push(tabLink)
                    if(tabIndex == 0) {
                        tab.classList.add("selected")
                        tabLink.classList.add("active")
                    }
                })
                let tabsOverview = document.createElement("div")
                tabsOverview.classList.add("tabs-overview")
                let tabsOverviewContainer = document.createElement("div")
                tabsOverviewContainer.classList.add("tabs-overview-container")
                tabLinkList.forEach((tabLink) => {
                    tabsOverview.appendChild(tabLink)
                })
                tabsOverviewContainer.appendChild(tabsOverview)
                tabbed.before(tabsOverviewContainer)

                function resize() {
                    let maxTabHeight = 0
                    tabbed.querySelectorAll(":scope > ul > li").forEach((tab, tabIndex) => {
                        let visibility = tab.style.display
                        tab.style.display = "block"
                        maxTabHeight = Math.max(tab.offsetHeight, maxTabHeight)
                        tab.style.display = visibility
                    })
                    tabbed.style.height = `${maxTabHeight + 10}px`
                }

                resize()
                new ResizeObserver(resize).observe(tabbed)
            })
        })
        
    }

    static resize(tabbed) {
        
    }
}