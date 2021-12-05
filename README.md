## Auto-generating your own plugin

You will find a helper script in `gst-plugins/tools/make_element` to generate
the source/header files for a new plugin.

To create sources for `myfilter` based on the `gsttransform` template run:

``` shell
cd gst-plugin/tools
bash make_element.sh myfilter gsttransform
```

This will create `gstmyfilter.c` and `gstmyfilter.h`. Open them in an editor and
start editing. There are several occurances of the string `template`, update
those with real values. The plugin will be called `myfilter` and it will have
one element called `myfilter` too. Also look for `FIXME:` markers that point you
to places where you need to edit the code.