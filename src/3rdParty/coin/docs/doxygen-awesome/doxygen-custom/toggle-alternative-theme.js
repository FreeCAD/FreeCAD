
let original_theme_active = true;

function toggle_alternative_theme() {
    if(original_theme_active) {
        document.documentElement.classList.add("alternative")
        original_theme_active = false;
    } else {
        document.documentElement.classList.remove("alternative")
        original_theme_active = true;
    }
}