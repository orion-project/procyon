# Images

### `./icon`
Images that are used as window header icons.

### `./toolbar`
Images mainly intended to be displayed in toolbars buttons and push buttons.

### `./src`
Non-optimized SVG in Inkscape format that are sources for images in other directories.

## SVG Optimization

[svgcleaner](https://github.com/RazrFalcon/svgcleaner) used for reducing the size of SVG-files that are built into the application resources. Almost all its settings are by default excepting coordinate precision and indentation.

```bash
svgcleaner \
    --coordinates-precision=2 \
    --properties-precision=2 \
    --transforms-precision=2 \
    --paths-coordinates-precision=2 \
    --indent=0 \
    in.svg \
    out
