// most of this file is based on markdown-preview.vim
//

var renderer = new marked.Renderer();
var flagSign = "53bde44bb4f156e94a85723fe633c80b54f11f69";
var rFlagSign = flagSign.split('').reverse().join('');
var aPoint = '<a style="position: relative;" href="#'+ rFlagSign +'" id="'+ rFlagSign +'"></a>';

        renderer.heading = function(text, level, raw) {
            var result = '';
            if (text.indexOf(flagSign) !== -1) {
                text = text.replace(flagSign, '');
                raw = text;
                result = aPoint;
            }
            return result
                + '<h'
                + level
                + ' id="'
                + this.options.headerPrefix
                + raw.toLowerCase().replace(/[^\w]+/g, '-')
                + '">'
                + text
                + '</h'
                + level
                + '>\n';
        };

        renderer.html = function(html) {
            var i, len, line;
            html = html.split('\n');
            for(i = 0, len = html.length; i < len; i++) {
                line = html[i];
                if(line.indexOf(flagSign) !== -1) {
                    html[i] = line.replace(flagSign, '') + aPoint;
                }
            }
            return html.join('\n');
        };

        renderer.listitem = function(text) {
            text = text.replace(flagSign, aPoint);
            return '<li>' + text + '</li>\n';
        };

        renderer.paragraph = function(text) {
            text = text.replace(flagSign, aPoint);
            return '<p>' + text + '</p>\n';
        };

        renderer.tablerow = function(content) {
            content = content.replace(flagSign, aPoint);
            return '<tr>\n' + content + '</tr>\n';
        };

        renderer.codespan = function(text) {
            var result = '';
            if(text.indexOf(flagSign) !== -1) {
                text = text.replace(flagSign, '');
                result = aPoint;
            }
            return result + '<code>' + text + '</code>\n'
        };

        renderer.image = function(href, title, text) {
            var result = '';
            if(!!title && title.indexOf(flagSign) !== -1) {
                title = title.replace(flagSign, '');
                result = aPoint;
            }
            if(!!text && text.indexOf(flagSign) !== -1) {
                text = text.replace(flagSign, '');
                result = aPoint;
            }
            return result + rImage.call(renderer, href, title, text);
        };

        renderer.link = function(href, title, text) {
            var result = '';
            if(!!href && href.indexOf(flagSign) !== -1) {
                href = href.replace(flagSign, '');
                result = aPoint;
            }
            if(!!title && title.indexOf(flagSign) !== -1) {
                title = title.replace(flagSign, '');
                result = aPoint;
            }
            if(!!text && text.indexOf(flagSign) !== -1) {
                text = text.replace(flagSign, '');
                result = aPoint;
            }
            return result + rLink.call(renderer, href, title, text);
        };

marked.setOptions({
  renderer: renderer,
  gfm: true,
  tables: true,
  breaks: false,
  pedantic: false,
  sanitize: false,
  smartLists: true,
  smartypants: false
});

function preview(){
    document.getElementById('preview').innerHTML = marked(str);
    location.hash = '#' + rFlagSign;
}
