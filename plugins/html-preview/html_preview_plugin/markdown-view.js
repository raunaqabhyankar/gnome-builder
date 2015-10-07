// most of this file is based on markdown-preview.vim
//
var LINE_NUMBER_SIZE = 6;

var renderer = new marked.Renderer();
var proto = marked.Renderer.prototype;

var flagSign = "53bde44bb4f156e94a85723fe633c80b54f11f69";
var flagSign_len = flagSign.length;
var rFlagSign = flagSign.split('').reverse().join('');

function get_apoint(line_num) {
  var flag = rFlagSign + line_num;

  return '<a style="position: relative;" href="#' +
         flag +
         '" id="' +
         flag +
         '"></a>';
}

function get_line_num(text) {
  pos = text.indexOf(flagSign);

  return (pos !== -1) ? text.substring (pos +flagSign_len , pos + flagSign_len + LINE_NUMBER_SIZE) : '';
}

function escape_text(text, encode) {
  return text
         .replace(!encode ? /&(?!#?\w+;)/g : /&/g, '&amp;')
         .replace(/</g, '&lt;')
         .replace(/>/g, '&gt;')
         .replace(/"/g, '&quot;')
         .replace(/'/g, '&#39;');
}

function replace_all(text, left_bound, right_bound, escaped) {
  var i, len, line;
  var line_num = '';
  var raw = '';
  var cleaned_text;

  left_bound = left_bound || '';
  right_bound = right_bound || '';
  escaped = (typeof escaped !== 'undefined') ? escaped : false;

  text = text.split('\n');

  for(i = 0, len = text.length; i < len; i++) {
      line = text[i];
      line_num = get_line_num (line);
      if(line_num !== '') {
          cleaned_text = line.replace(flagSign + line_num, '');
          text[i] = get_apoint(line_num) +
                    left_bound +
                    (escaped ? escape_text(cleaned_text, true) : cleaned_text) +
                    right_bound;

          raw += cleaned_text + '\n';
      }
  }

  return {'tagged': text.join('\n'), 'raw': raw};
}

        renderer.heading = function(text, level, raw) {
            var new_text = replace_all(text);
            return  proto.heading.call (this, new_text.tagged, level, new_text.raw);
        };

        renderer.html = function(text) {
            return replace_all (text).tagged;
        };

        renderer.listitem = function(text) {
            return proto.listitem.call (this, replace_all(text).tagged);
        };

        renderer.paragraph = function(text) {
            return proto.paragraph.call (this, replace_all(text).tagged);
        };

        renderer.tablecell = function(content, flags) {
            return proto.tablecell.call (this, replace_all(content).tagged, flags);
        };

        renderer.codespan = function(text) {
            return proto.codespan.call (this, replace_all(text).tagged);
        };

        // webkit doesn't like link element in code, we need to trick
        renderer.code = function(code, lang, escaped) {
            return '<pre><code>' + replace_all(code, '<code>', '</code>', true).tagged + '</code></pre>';
        };

        renderer.image = function(href, title, text) {
            var line_num = get_line_num (text);
            if (line_num) {
              text = text.replace(flagSign + line_num, '');
              return  get_apoint(line_num) + proto.image.call(renderer, href, title, text);
            }

            return proto.image.call(renderer, href, title, text);
        };

        renderer.link = function(href, title, text) {
            var line_num = get_line_num (text);
            if (line_num) {
              text = text.replace(flagSign + line_num, '');
              return get_apoint(line_num) + proto.link.call(renderer, href, title, text);
            }

            return proto.link.call(renderer, href, title, text);
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
}
