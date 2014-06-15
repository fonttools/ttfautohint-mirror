// maingui.cpp

// Copyright (C) 2012-2014 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#include <config.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <QtGui>

#include "info.h"
#include "maingui.h"

#include <ttfautohint.h>


// XXX Qt 4.8 bug: locale->quoteString("foo")
//                 inserts wrongly encoded quote characters
//                 into rich text QString
#if HAVE_QT_QUOTESTRING
#  define QUOTE_STRING(x) locale->quoteString(x)
#  define QUOTE_STRING_LITERAL(x) locale->quoteString(x)
#else
#  define QUOTE_STRING(x) "\"" + x + "\""
#  define QUOTE_STRING_LITERAL(x) "\"" x "\""
#endif


// the available script tags and its descriptions are directly extracted
// from `ttfautohint-scripts.h'
typedef struct Script_Names_
{
  const char* tag;
  const char* description;
} Script_Names;

#undef SCRIPT
#define SCRIPT(s, S, d, h, sc1, sc2, sc3) \
          {#s, d},

const Script_Names script_names[] =
{
#include <ttfautohint-scripts.h>
  {NULL, NULL}
};


Main_GUI::Main_GUI(bool horizontal_layout,
                   int range_min,
                   int range_max,
                   int limit,
                   bool gray,
                   bool gdi,
                   bool dw,
                   int increase,
                   const char* exceptions,
                   int stem_width,
                   bool ignore,
                   bool wincomp,
                   bool pre,
                   bool composites,
                   bool no,
                   const char* dflt,
                   const char* fallback,
                   bool symb,
                   bool dh)
: hinting_range_min(range_min),
  hinting_range_max(range_max),
  hinting_limit(limit),
  gray_strong_stem_width(gray),
  gdi_cleartype_strong_stem_width(gdi),
  dw_cleartype_strong_stem_width(dw),
  increase_x_height(increase),
  x_height_snapping_exceptions_string(exceptions),
  fallback_stem_width(stem_width),
  ignore_restrictions(ignore),
  windows_compatibility(wincomp),
  pre_hinting(pre),
  hint_composites(composites),
  no_info(no),
  symbol(symb),
  dehint(dh)
{
  int i;

  // map default script tag to an index,
  // replacing an invalid one with the default value
  int latn_script_idx = 0;
  for (i = 0; script_names[i].tag; i++)
  {
    if (!strcmp("latn", script_names[i].tag))
      latn_script_idx = i;
    if (!strcmp(dflt, script_names[i].tag))
      break;
  }
  default_script_idx = script_names[i].tag ? i : latn_script_idx;

  // map fallback script tag to an index,
  // replacing an invalid one with the default value
  int none_script_idx = 0;
  for (i = 0; script_names[i].tag; i++)
  {
    if (!strcmp("none", script_names[i].tag))
      none_script_idx = i;
    if (!strcmp(fallback, script_names[i].tag))
      break;
  }
  fallback_script_idx = script_names[i].tag ? i : none_script_idx;

  x_height_snapping_exceptions = NULL;

  create_layout(horizontal_layout);
  create_connections();
  create_actions();
  create_menus();
  create_status_bar();

  set_defaults();
  read_settings();

  setUnifiedTitleAndToolBarOnMac(true);

  // XXX register translations somewhere and loop over them
  if (QLocale::system().name() == "en_US")
    locale = new QLocale;
  else
    locale = new QLocale(QLocale::C);
}


Main_GUI::~Main_GUI()
{
  number_set_free(x_height_snapping_exceptions);
}


// overloading

void
Main_GUI::closeEvent(QCloseEvent* event)
{
  write_settings();
  event->accept();
}


void
Main_GUI::about()
{
  QMessageBox::about(this,
                     tr("About TTFautohint"),
                     tr("<p>This is <b>TTFautohint</b> version %1<br>"
                        " Copyright %2 2011-2014<br>"
                        " by Werner Lemberg <tt>&lt;wl@gnu.org&gt;</tt></p>"
                        ""
                        "<p><b>TTFautohint</b> adds new auto-generated hints"
                        " to a TrueType font or TrueType collection.</p>"
                        ""
                        "<p>License:"
                        " <a href='http://www.freetype.org/FTL.TXT'>FreeType"
                        " License (FTL)</a> or"
                        " <a href='http://www.freetype.org/GPL.TXT'>GNU"
                        " GPLv2</a></p>")
                        .arg(VERSION)
                        .arg(QChar(0xA9)));
}


void
Main_GUI::browse_input()
{
  // XXX remember last directory
  QString file = QFileDialog::getOpenFileName(
                   this,
                   tr("Open Input File"),
                   QDir::homePath(),
                   "");
  if (!file.isEmpty())
    input_line->setText(QDir::toNativeSeparators(file));
}


void
Main_GUI::browse_output()
{
  // XXX remember last directory
  QString file = QFileDialog::getSaveFileName(
                   this,
                   tr("Open Output File"),
                   QDir::homePath(),
                   "");

  if (!file.isEmpty())
    output_line->setText(QDir::toNativeSeparators(file));
}


void
Main_GUI::check_min()
{
  int min = min_box->value();
  int max = max_box->value();
  int limit = limit_box->value();
  if (min > max)
    max_box->setValue(min);
  if (min > limit)
    limit_box->setValue(min);
}


void
Main_GUI::check_max()
{
  int min = min_box->value();
  int max = max_box->value();
  int limit = limit_box->value();
  if (max < min)
    min_box->setValue(max);
  if (max > limit)
    limit_box->setValue(max);
}


void
Main_GUI::check_limit()
{
  int min = min_box->value();
  int max = max_box->value();
  int limit = limit_box->value();
  if (limit < max)
    max_box->setValue(limit);
  if (limit < min)
    min_box->setValue(limit);
}


void
Main_GUI::check_dehint()
{
  if (dehint_box->isChecked())
  {
    min_label->setEnabled(false);
    min_box->setEnabled(false);

    max_label->setEnabled(false);
    max_box->setEnabled(false);

    default_label->setEnabled(false);
    default_box->setEnabled(false);
    fallback_label->setEnabled(false);
    fallback_box->setEnabled(false);

    no_limit_box->setEnabled(false);
    limit_label->setEnabled(false);
    limit_box->setEnabled(false);

    no_increase_box->setEnabled(false);
    increase_label->setEnabled(false);
    increase_box->setEnabled(false);

    snapping_label->setEnabled(false);
    snapping_line->setEnabled(false);

    default_stem_width_box->setEnabled(false);
    stem_width_label->setEnabled(false);
    stem_width_box->setEnabled(false);

    wincomp_box->setEnabled(false);
    pre_box->setEnabled(false);
    hint_box->setEnabled(false);
    symbol_box->setEnabled(false);

    stem_label->setEnabled(false);
    gray_box->setEnabled(false);
    gdi_box->setEnabled(false);
    dw_box->setEnabled(false);
  }
  else
  {
    min_label->setEnabled(true);
    min_box->setEnabled(true);

    max_label->setEnabled(true);
    max_box->setEnabled(true);

    default_label->setEnabled(true);
    default_box->setEnabled(true);
    fallback_label->setEnabled(true);
    fallback_box->setEnabled(true);

    no_limit_box->setEnabled(true);
    check_no_limit();

    no_increase_box->setEnabled(true);
    check_no_increase();

    snapping_label->setEnabled(true);
    snapping_line->setEnabled(true);

    default_stem_width_box->setEnabled(true);
    check_default_stem_width();

    wincomp_box->setEnabled(true);
    pre_box->setEnabled(true);
    hint_box->setEnabled(true);
    symbol_box->setEnabled(true);

    stem_label->setEnabled(true);
    gray_box->setEnabled(true);
    gdi_box->setEnabled(true);
    dw_box->setEnabled(true);
  }
}


void
Main_GUI::check_no_limit()
{
  if (no_limit_box->isChecked())
  {
    limit_label->setEnabled(false);
    limit_label->setText(limit_label_text);
    limit_box->setEnabled(false);
    no_limit_box->setText(no_limit_box_text_with_key);
  }
  else
  {
    limit_label->setEnabled(true);
    limit_label->setText(limit_label_text_with_key);
    limit_box->setEnabled(true);
    no_limit_box->setText(no_limit_box_text);
  }
}


void
Main_GUI::check_no_increase()
{
  if (no_increase_box->isChecked())
  {
    increase_label->setEnabled(false);
    increase_label->setText(increase_label_text);
    increase_box->setEnabled(false);
    no_increase_box->setText(no_increase_box_text_with_key);
  }
  else
  {
    increase_label->setEnabled(true);
    increase_label->setText(increase_label_text_with_key);
    increase_box->setEnabled(true);
    no_increase_box->setText(no_increase_box_text);
  }
}


void
Main_GUI::check_default_stem_width()
{
  if (default_stem_width_box->isChecked())
  {
    stem_width_label->setEnabled(false);
    stem_width_label->setText(stem_width_label_text);
    stem_width_box->setEnabled(false);
    default_stem_width_box->setText(default_stem_width_box_text_with_key);
  }
  else
  {
    stem_width_label->setEnabled(true);
    stem_width_label->setText(stem_width_label_text_with_key);
    stem_width_box->setEnabled(true);
    default_stem_width_box->setText(default_stem_width_box_text);
  }
}


void
Main_GUI::check_run()
{
  if (input_line->text().isEmpty() || output_line->text().isEmpty())
    run_button->setEnabled(false);
  else
    run_button->setEnabled(true);
}


void
Main_GUI::absolute_input()
{
  QString input_name = QDir::fromNativeSeparators(input_line->text());
  if (!input_name.isEmpty()
      && QDir::isRelativePath(input_name))
  {
    QDir cur_path(QDir::currentPath() + "/" + input_name);
    input_line->setText(QDir::toNativeSeparators(cur_path.absolutePath()));
  }
}


void
Main_GUI::absolute_output()
{
  QString output_name = QDir::fromNativeSeparators(output_line->text());
  if (!output_name.isEmpty()
      && QDir::isRelativePath(output_name))
  {
    QDir cur_path(QDir::currentPath() + "/" + output_name);
    output_line->setText(QDir::toNativeSeparators(cur_path.absolutePath()));
  }
}


void
Main_GUI::check_number_set()
{
  QString text = snapping_line->text();
  QString qs;

  // construct ASCII string from arbitrary Unicode data;
  // the idea is to accept, say, CJK fullwidth digits also
  for (int i = 0; i < text.size(); i++)
  {
    QChar c = text.at(i);

    int digit = c.digitValue();
    if (digit >= 0)
      qs += QString::number(digit);
    else if (c.isSpace())
      qs += ' ';
    // U+30FC KATAGANA-HIRAGANA PROLONGED SOUND MARK is assigned
    // to the `-' key in some Japanese input methods
    else if (c.category() == QChar::Punctuation_Dash
             || c == QChar(0x30FC))
      qs += '-';
    // various Unicode COMMA characters,
    // including representation forms
    else if (c == QChar(',')
             || c == QChar(0x055D)
             || c == QChar(0x060C)
             || c == QChar(0x07F8)
             || c == QChar(0x1363)
             || c == QChar(0x1802)
             || c == QChar(0x1808)
             || c == QChar(0x3001)
             || c == QChar(0xA4FE)
             || c == QChar(0xA60D)
             || c == QChar(0xA6F5)
             || c == QChar(0xFE10)
             || c == QChar(0xFE11)
             || c == QChar(0xFE50)
             || c == QChar(0xFE51)
             || c == QChar(0xFF0C)
             || c == QChar(0xFF64))
      qs += ',';
    else
      qs += c; // we do error handling below
  }

  if (x_height_snapping_exceptions)
    number_set_free(x_height_snapping_exceptions);

  QByteArray str = qs.toLocal8Bit();
  const char* s = number_set_parse(str.constData(),
                                   &x_height_snapping_exceptions,
                                   6, 0x7FFF);
  if (s && *s)
  {
    statusBar()->setStyleSheet("color: red;");
    if (x_height_snapping_exceptions == NUMBERSET_ALLOCATION_ERROR)
      statusBar()->showMessage(
        tr("allocation error"));
    else if (x_height_snapping_exceptions == NUMBERSET_INVALID_CHARACTER)
      statusBar()->showMessage(
        tr("invalid character (use digits, dashes, commas, and spaces)"));
    else if (x_height_snapping_exceptions == NUMBERSET_OVERFLOW)
      statusBar()->showMessage(
        tr("overflow"));
    else if (x_height_snapping_exceptions == NUMBERSET_INVALID_RANGE)
      statusBar()->showMessage(
        tr("invalid range (minimum is 6, maximum is 32767)"));
    else if (x_height_snapping_exceptions == NUMBERSET_OVERLAPPING_RANGES)
      statusBar()->showMessage(
        tr("overlapping ranges"));
    else if (x_height_snapping_exceptions == NUMBERSET_NOT_ASCENDING)
      statusBar()->showMessage(
        tr("values und ranges must be specified in ascending order"));

    snapping_line->setText(qs);
    snapping_line->setFocus(Qt::OtherFocusReason);
    snapping_line->setCursorPosition(s - str.constData());

    x_height_snapping_exceptions = NULL;
  }
  else
  {
    // normalize if there is no error
    char* new_str = number_set_show(x_height_snapping_exceptions,
                                    6, 0x7FFF);
    snapping_line->setText(new_str);
    free(new_str);
  }
}


void
Main_GUI::clear_status_bar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
}


int
Main_GUI::check_filenames(const QString& input_name,
                          const QString& output_name)
{
  if (!QFile::exists(input_name))
  {
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The file %1 cannot be found.")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(input_name))),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  if (input_name == output_name)
  {
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("Input and output file names must be different."),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  if (QFile::exists(output_name))
  {
    int ret = QMessageBox::warning(
                this,
                "TTFautohint",
                tr("The file %1 already exists.\n"
                   "Overwrite?")
                   .arg(QUOTE_STRING(QDir::toNativeSeparators(output_name))),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
    if (ret == QMessageBox::No)
      return 0;
  }

  return 1;
}


int
Main_GUI::open_files(const QString& input_name,
                     FILE** in,
                     const QString& output_name,
                     FILE** out)
{
  const int buf_len = 1024;
  char buf[buf_len];

  *in = fopen(qPrintable(input_name), "rb");
  if (!*in)
  {
    strerror_r(errno, buf, buf_len);
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The following error occurred while opening input file %1:\n")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(input_name)))
        + QString::fromLocal8Bit(buf),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  *out = fopen(qPrintable(output_name), "wb");
  if (!*out)
  {
    strerror_r(errno, buf, buf_len);
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The following error occurred while opening output file %1:\n")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(output_name)))
        + QString::fromLocal8Bit(buf),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  return 1;
}


extern "C" {

struct GUI_Progress_Data
{
  long last_sfnt;
  bool begin;
  QProgressDialog* dialog;
};


int
gui_progress(long curr_idx,
             long num_glyphs,
             long curr_sfnt,
             long num_sfnts,
             void* user)
{
  GUI_Progress_Data* data = static_cast<GUI_Progress_Data*>(user);

  if (num_sfnts > 1 && curr_sfnt != data->last_sfnt)
  {
    data->dialog->setLabelText(QCoreApplication::translate(
                                 "GuiProgress",
                                 "Auto-hinting subfont %1 of %2"
                                 " with %3 glyphs...")
                               .arg(curr_sfnt + 1)
                               .arg(num_sfnts)
                               .arg(num_glyphs));

    if (curr_sfnt + 1 == num_sfnts)
    {
      data->dialog->setAutoReset(true);
      data->dialog->setAutoClose(true);
    }
    else
    {
      data->dialog->setAutoReset(false);
      data->dialog->setAutoClose(false);
    }

    data->last_sfnt = curr_sfnt;
    data->begin = true;
  }

  if (data->begin)
  {
    if (num_sfnts == 1)
      data->dialog->setLabelText(QCoreApplication::translate(
                                   "GuiProgress",
                                   "Auto-hinting %1 glyphs...")
                                 .arg(num_glyphs));
    data->dialog->setMaximum(num_glyphs - 1);

    data->begin = false;
  }

  data->dialog->setValue(curr_idx);

  if (data->dialog->wasCanceled())
    return 1;

  return 0;
}

} // extern "C"


// return value 1 indicates a retry

int
Main_GUI::handle_error(TA_Error error,
                       const unsigned char* error_string,
                       QString output_name)
{
  int ret = 0;

  if (error == TA_Err_Canceled)
     ;
  else if (error == TA_Err_Invalid_FreeType_Version)
    QMessageBox::critical(
      this,
      "TTFautohint",
      tr("FreeType version 2.4.5 or higher is needed.\n"
         "Are you perhaps using a wrong FreeType DLL?"),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Invalid_Font_Type)
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("This font is not a valid font"
         " in SFNT format with TrueType outlines.\n"
         "In particular, CFF outlines are not supported."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Already_Processed)
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("This font has already been processed by TTFautohint."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Legal_Permission)
  {
    int yesno = QMessageBox::warning(
                  this,
                  "TTFautohint",
                  tr("Bit 1 in the %1 field of the %2 table is set:"
                     " This font must not be modified"
                     " without permission of the legal owner.\n"
                     "Do you have such a permission?")
                     .arg(QUOTE_STRING_LITERAL("fsType"))
                     .arg(QUOTE_STRING_LITERAL("OS/2")),
                  QMessageBox::Yes | QMessageBox::No,
                  QMessageBox::No);
    if (yesno == QMessageBox::Yes)
    {
      ignore_restrictions = true;
      ret = 1;
    }
  }
  else if (error == TA_Err_Missing_Unicode_CMap)
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The input font doesn't contain a Unicode character map.\n"
         "Maybe you haven't set the %1 checkbox?")
         .arg(QUOTE_STRING_LITERAL(tr("Symbol Font"))),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Symbol_CMap)
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The input font does neither contain a symbol"
         " nor a character map."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Glyph)
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("No glyph for a standard character"
         " to derive standard width and height.\n"
         "Please check the documentation for a list of"
         " script-specific standard characters.\n"
         "\n"
         "Set the %1 checkbox if you want to circumvent this test.")
         .arg(QUOTE_STRING_LITERAL(tr("Symbol Font"))),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("Error code 0x%1 while autohinting font:\n")
         .arg(error, 2, 16, QLatin1Char('0'))
        + QString::fromLocal8Bit((const char*)error_string),
      QMessageBox::Ok,
      QMessageBox::Ok);

  if (QFile::exists(output_name) && remove(qPrintable(output_name)))
  {
    const int buf_len = 1024;
    char buf[buf_len];

    strerror_r(errno, buf, buf_len);
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The following error occurred while removing output file %1:\n")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(output_name)))
        + QString::fromLocal8Bit(buf),
      QMessageBox::Ok,
      QMessageBox::Ok);
  }

  return ret;
}


void
Main_GUI::run()
{
  statusBar()->clearMessage();

  QString input_name = QDir::fromNativeSeparators(input_line->text());
  QString output_name = QDir::fromNativeSeparators(output_line->text());
  if (!check_filenames(input_name, output_name))
    return;

  // we need C file descriptors for communication with TTF_autohint
  FILE* input;
  FILE* output;

again:
  if (!open_files(input_name, &input, output_name, &output))
    return;

  QProgressDialog dialog;
  dialog.setCancelButtonText(tr("Cancel"));
  dialog.setMinimumDuration(1000);
  dialog.setWindowModality(Qt::WindowModal);

  const unsigned char* error_string;
  TA_Info_Func info_func = info;
  GUI_Progress_Data gui_progress_data = {-1, true, &dialog};
  Info_Data info_data;

  info_data.data = NULL; // must be deallocated after use
  info_data.data_wide = NULL; // must be deallocated after use
  info_data.data_len = 0;
  info_data.data_wide_len = 0;

  info_data.hinting_range_min = min_box->value();
  info_data.hinting_range_max = max_box->value();
  info_data.hinting_limit = no_limit_box->isChecked()
                            ? 0
                            : limit_box->value();

  info_data.gray_strong_stem_width = gray_box->isChecked();
  info_data.gdi_cleartype_strong_stem_width = gdi_box->isChecked();
  info_data.dw_cleartype_strong_stem_width = dw_box->isChecked();

  info_data.increase_x_height = no_increase_box->isChecked()
                                ? 0
                                : increase_box->value();
  info_data.x_height_snapping_exceptions = x_height_snapping_exceptions;
  info_data.fallback_stem_width = default_stem_width_box->isChecked()
                                  ? 0
                                  : stem_width_box->value();

  info_data.windows_compatibility = wincomp_box->isChecked();
  info_data.pre_hinting = pre_box->isChecked();
  info_data.hint_composites = hint_box->isChecked();
  info_data.symbol = symbol_box->isChecked();
  info_data.dehint = dehint_box->isChecked();

  strncpy(info_data.default_script,
          script_names[default_box->currentIndex()].tag,
          sizeof (info_data.default_script));
  strncpy(info_data.fallback_script,
          script_names[fallback_box->currentIndex()].tag,
          sizeof (info_data.fallback_script));

  if (info_box->isChecked())
  {
    int ret = build_version_string(&info_data);
    if (ret == 1)
      QMessageBox::information(
        this,
        "TTFautohint",
        tr("Can't allocate memory for <b>TTFautohint</b> options string"
           " in <i>name</i> table."),
        QMessageBox::Ok,
        QMessageBox::Ok);
    else if (ret == 2)
      QMessageBox::information(
        this,
        "TTFautohint",
        tr("<b>TTFautohint</b> options string"
           " in <i>name</i> table too long."),
        QMessageBox::Ok,
        QMessageBox::Ok);
  }
  else
    info_func = NULL;

  if (info_data.symbol
      && info_data.fallback_stem_width
      && !strcmp(info_data.fallback_script, "none"))
    QMessageBox::information(
      this,
      "TTFautohint",
      tr("Setting a fallback stem width for a symbol font"
         " without setting a fallback script has no effect."),
      QMessageBox::Ok,
      QMessageBox::Ok);


  QByteArray snapping_string = snapping_line->text().toLocal8Bit();

  TA_Error error =
    TTF_autohint("in-file, out-file,"
                 "hinting-range-min, hinting-range-max,"
                 "hinting-limit,"
                 "gray-strong-stem-width,"
                 "gdi-cleartype-strong-stem-width,"
                 "dw-cleartype-strong-stem-width,"
                 "error-string,"
                 "progress-callback, progress-callback-data,"
                 "info-callback, info-callback-data,"
                 "ignore-restrictions,"
                 "windows-compatibility,"
                 "pre-hinting,"
                 "hint-composites,"
                 "increase-x-height,"
                 "x-height-snapping-exceptions, fallback-stem-width,"
                 "default-script, fallback-script,"
                 "symbol, dehint",
                 input, output,
                 info_data.hinting_range_min, info_data.hinting_range_max,
                 info_data.hinting_limit,
                 info_data.gray_strong_stem_width,
                 info_data.gdi_cleartype_strong_stem_width,
                 info_data.dw_cleartype_strong_stem_width,
                 &error_string,
                 gui_progress, &gui_progress_data,
                 info_func, &info_data,
                 ignore_restrictions,
                 info_data.windows_compatibility,
                 info_data.pre_hinting,
                 info_data.hint_composites,
                 info_data.increase_x_height,
                 snapping_string.constData(), info_data.fallback_stem_width,
                 info_data.default_script, info_data.fallback_script,
                 info_data.symbol, info_data.dehint);

  if (info_box->isChecked())
  {
    free(info_data.data);
    free(info_data.data_wide);
  }

  fclose(input);
  fclose(output);

  if (error)
  {
    if (handle_error(error, error_string, output_name))
      goto again;
  }
  else
    statusBar()->showMessage(tr("Auto-hinting finished."));
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
Main_GUI::create_layout(bool horizontal_layout)
{
  //
  // file stuff
  //
  QCompleter* completer = new QCompleter(this);
  QFileSystemModel* model = new QFileSystemModel(completer);
  model->setRootPath(QDir::rootPath());
  completer->setModel(model);

  input_label = new QLabel(tr("&Input File:"));
  input_line = new Drag_Drop_Line_Edit;
  input_button = new QPushButton(tr("Browse..."));
  input_label->setBuddy(input_line);
  // enforce rich text to get nice word wrapping
  input_label->setToolTip(
    tr("<b></b>The input file, either a TrueType font (TTF),"
       " TrueType collection (TTC), or a TrueType-based OpenType font."));
  input_line->setCompleter(completer);

  output_label = new QLabel(tr("&Output File:"));
  output_line = new Drag_Drop_Line_Edit;
  output_button = new QPushButton(tr("Browse..."));
  output_label->setBuddy(output_line);
  output_label->setToolTip(
    tr("<b></b>The output file, which will be essentially identical"
       " to the input font but will contain new, generated hints."));
  output_line->setCompleter(completer);

  //
  // minmax controls
  //
  min_label = new QLabel(tr("Hint Set Range Mi&nimum:"));
  min_box = new QSpinBox;
  min_label->setBuddy(min_box);
  min_label->setToolTip(
    tr("The minimum PPEM value of the range for which"
       " <b>TTFautohint</b> computes <i>hint sets</i>."
       " A hint set for a given PPEM value hints this size optimally."
       " The larger the range, the more hint sets are considered,"
       " usually increasing the size of the bytecode.<br>"
       "Note that changing this range doesn't influence"
       " the <i>gasp</i> table:"
       " Hinting is enabled for all sizes."));
  min_box->setKeyboardTracking(false);
  min_box->setRange(2, 10000);

  max_label = new QLabel(tr("Hint Set Range Ma&ximum:"));
  max_box = new QSpinBox;
  max_label->setBuddy(max_box);
  max_label->setToolTip(
    tr("The maximum PPEM value of the range for which"
       " <b>TTFautohint</b> computes <i>hint sets</i>."
       " A hint set for a given PPEM value hints this size optimally."
       " The larger the range, the more hint sets are considered,"
       " usually increasing the size of the bytecode.<br>"
       "Note that changing this range doesn't influence"
       " the <i>gasp</i> table:"
       " Hinting is enabled for all sizes."));
  max_box->setKeyboardTracking(false);
  max_box->setRange(2, 10000);

  //
  // OpenType default script
  //
  default_label = new QLabel(tr("Defa&ult Script:"));
  default_box = new QComboBox;
  default_label->setBuddy(default_box);
  default_label->setToolTip(
    tr("This sets the default script for OpenType features:"
       "  After applying all features that are handled specially"
       " (for example small caps or superscript glyphs),"
       " <b>TTFautohint</b> uses this value for the remaining features."));
  for (int i = 0; script_names[i].tag; i++)
  {
    // XXX: how to provide translations?
    default_box->insertItem(i,
                            QString("%1 (%2)")
                                    .arg(script_names[i].tag)
                                    .arg(script_names[i].description));
  }

  //
  // hinting and fallback controls
  //
  fallback_label = new QLabel(tr("Fallback &Script:"));
  fallback_box = new QComboBox;
  fallback_label->setBuddy(fallback_box);
  fallback_label->setToolTip(
    tr("This sets the fallback script for glyphs"
       " that <b>TTFautohint</b> can't map to a script automatically."));
  for (int i = 0; script_names[i].tag; i++)
  {
    // XXX: how to provide translations?
    fallback_box->insertItem(i,
                             QString("%1 (%2)")
                                     .arg(script_names[i].tag)
                                     .arg(script_names[i].description));
  }

  //
  // hinting limit
  //
  limit_label_text_with_key = tr("Hinting &Limit:");
  limit_label_text = tr("Hinting Limit:");
  limit_label = new QLabel(limit_label_text_with_key);
  limit_box = new QSpinBox;
  limit_label->setBuddy(limit_box);
  limit_label->setToolTip(
    tr("Make <b>TTFautohint</b> add bytecode to the output font so that"
       " sizes larger than this PPEM value are not hinted"
       " (regardless of the values in the <i>gasp</i> table)."));
  limit_box->setKeyboardTracking(false);
  limit_box->setRange(2, 10000);

  no_limit_box_text_with_key = tr("No Hinting &Limit");
  no_limit_box_text = tr("No Hinting Limit");
  no_limit_box = new QCheckBox(no_limit_box_text, this);
  no_limit_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> adds no hinting limit"
       " to the bytecode."));

  //
  // x height increase limit
  //
  increase_label_text_with_key = tr("x Height In&crease Limit:");
  increase_label_text = tr("x Height Increase Limit:");
  increase_label = new QLabel(increase_label_text_with_key);
  increase_box = new QSpinBox;
  increase_label->setBuddy(increase_box);
  increase_label->setToolTip(
    tr("For PPEM values in the range 5&nbsp;&lt; PPEM &lt;&nbsp;<i>n</i>,"
       " where <i>n</i> is the value selected by this spin box,"
       " round up the font's x&nbsp;height much more often than normally.<br>"
       "Use this if holes in letters like <i>e</i> get filled,"
       " for example."));
  increase_box->setKeyboardTracking(false);
  increase_box->setRange(6, 10000);

  no_increase_box_text_with_key = tr("No x Height In&crease");
  no_increase_box_text = tr("No x Height Increase");
  no_increase_box = new QCheckBox(no_increase_box_text, this);
  no_increase_box->setToolTip(
    tr("If switched on,"
       " <b>TTFautohint</b> does not increase the x&nbsp;height."));

  //
  // x height snapping exceptions
  //
  snapping_label = new QLabel(tr("x Height Snapping Excep&tions:"));
  snapping_line = new Tooltip_Line_Edit;
  snapping_label->setBuddy(snapping_line);
  snapping_label->setToolTip(
    tr("<p>A list of comma separated PPEM values or value ranges"
       " at which no x&nbsp;height snapping shall be applied"
       " (x&nbsp;height snapping usually slightly increases"
       " the size of all glyphs).</p>"
       ""
       "Examples:<br>"
       "&nbsp;&nbsp;<tt>2, 3-5, 12-17</tt><br>"
       "&nbsp;&nbsp;<tt>-20, 40-</tt>"
       " (meaning PPEM &le; 20 or PPEM &ge; 40)<br>"
       "&nbsp;&nbsp;<tt>-</tt> (meaning all possible PPEM values)"));

  //
  // fallback stem width
  //
  stem_width_label_text_with_key = tr("Fall&back Stem Width:");
  stem_width_label_text = tr("Fallback Stem Width:");
  stem_width_label = new QLabel(stem_width_label_text_with_key);
  stem_width_box = new QSpinBox;
  stem_width_label->setBuddy(stem_width_box);
  stem_width_label->setToolTip(
    tr("Set horizontal stem width (in font units) for all scripts"
       " that lack proper standard characters in the font.<br>"
       "If not set, <b>TTFautohint</b> uses a hard-coded default value."));
  stem_width_box->setKeyboardTracking(false);
  stem_width_box->setRange(1, 10000);

  default_stem_width_box_text_with_key = tr("Default Fall&back Stem Width");
  default_stem_width_box_text = tr("Default Fallback Stem Width");
  default_stem_width_box = new QCheckBox(default_stem_width_box_text, this);
  default_stem_width_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> uses a default value"
       " for the fallback stem width (50 font units at 2048 UPEM)."));

  //
  // flags
  //
  wincomp_box = new QCheckBox(tr("Windows Com&patibility"), this);
  wincomp_box->setToolTip(
    tr("If switched on, add two artificial blue zones positioned at the"
       " <tt>usWinAscent</tt> and <tt>usWinDescent</tt> values"
       " (from the font's <i>OS/2</i> table)."
       "  This option, usually in combination"
       " with value <tt>-</tt> (a single dash)"
       " for the <i>x&nbsp;Height Snapping Exceptions</i> option,"
       " should be used if those two <i>OS/2</i> values are tight,"
       " and you are experiencing clipping during rendering."));

  pre_box = new QCheckBox(tr("Pr&e-hinting"), this);
  pre_box->setToolTip(
    tr("If switched on, the original bytecode of the input font"
       " gets applied (at EM size, usually 2048ppem)"
       " to derive the glyph outlines for <b>TTFautohint</b>."
       "  Note that the original bytecode will always be discarded."));

  hint_box = new QCheckBox(tr("Hint Co&mposites")
                           + "                ", this); // make label wider
  hint_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> hints composite glyphs"
       " as a whole, including subglyphs."
       "  Otherwise, glyph components get hinted separately.<br>"
       "Deactivating this flag reduces the bytecode size enormously,"
       " however, it might yield worse results."));

  symbol_box = new QCheckBox(tr("S&ymbol Font"), this);
  symbol_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> accepts fonts"
       " that don't contain a single standard character"
       " for any of the supported scripts.<br>"
       "Use this for symbol or dingbat fonts, for example."));

  dehint_box = new QCheckBox(tr("&Dehint"), this);
  dehint_box->setToolTip(
    tr("<b></b>If set, remove all hints from the font."));

  info_box = new QCheckBox(tr("Add ttf&autohint Info"), this);
  info_box->setToolTip(
    tr("If switched on, information about <b>TTFautohint</b>"
       " and its calling parameters are added to the version string(s)"
       " (name ID&nbsp;5) in the <i>name</i> table."));

  //
  // stem width and positioning
  //
  stem_label = new QLabel(tr("Stron&g Stem Width and Positioning:"));
  stem_label->setToolTip(
    tr("<b>TTFautohint</b> provides two different hinting algorithms"
       " that can be selected for various hinting modes."
       ""
       "<p><i>strong</i> (checkbox set):"
       " Position horizontal stems and snap stem widths"
       " to integer pixel values.  While making the output look crisper,"
       " outlines become more distorted.</p>"
       ""
       "<p><i>smooth</i> (checkbox not set):"
       " Use discrete values for horizontal stems and stem widths."
       "  This only slightly increases the contrast"
       " but avoids larger outline distortion.</p>"));

  gray_box = new QCheckBox(tr("Grayscale"), this);
  gray_box->setToolTip(
    tr("<b></b>Grayscale rendering, no ClearType activated."));
  stem_label->setBuddy(gray_box);

  gdi_box = new QCheckBox(tr("GDI ClearType"), this);
  gdi_box->setToolTip(
    tr("GDI ClearType rendering,"
       " introduced in 2000 for Windows XP.<br>"
       "The rasterizer version (as returned by the"
       " GETINFO bytecode instruction) is in the range"
       " 36&nbsp;&le; version &lt;&nbsp;38, and ClearType is enabled.<br>"
       "Along the vertical axis, this mode behaves like B/W rendering."));

  dw_box = new QCheckBox(tr("DW ClearType"), this);
  dw_box->setToolTip(
    tr("DirectWrite ClearType rendering,"
       " introduced in 2008 for Windows Vista.<br>"
       "The rasterizer version (as returned by the"
       " GETINFO bytecode instruction) is &ge;&nbsp;38,"
       " ClearType is enabled, and subpixel positioning is enabled also.<br>"
       "Smooth rendering along the vertical axis."));

  //
  // running
  //
  run_button = new QPushButton("    "
                               + tr("&Run")
                               + "    "); // make label wider

  if (horizontal_layout)
    create_horizontal_layout();
  else
    create_vertical_layout();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
Main_GUI::create_vertical_layout()
{
  // top area
  QGridLayout* file_layout = new QGridLayout;

  file_layout->addWidget(input_label, 0, 0, Qt::AlignRight);
  file_layout->addWidget(input_line, 0, 1);
  file_layout->addWidget(input_button, 0, 2);

  file_layout->setRowStretch(1, 1);

  file_layout->addWidget(output_label, 2, 0, Qt::AlignRight);
  file_layout->addWidget(output_line, 2, 1);
  file_layout->addWidget(output_button, 2, 2);

  //
  // the whole gui
  //
  QGridLayout* gui_layout = new QGridLayout;
  QFrame* hline = new QFrame;
  hline->setFrameShape(QFrame::HLine);
  int row = 0; // this counter simplifies inserting new items

  gui_layout->setRowMinimumHeight(row, 10); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addLayout(file_layout, row, 0, row, -1);
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(hline, row, 0, row, -1);
  gui_layout->setRowStretch(row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(min_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(min_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(max_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(max_box, row++, 1, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(default_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(default_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(fallback_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(fallback_box, row++, 1, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(limit_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(limit_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(no_limit_box, row++, 1);

  gui_layout->addWidget(increase_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(increase_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(no_increase_box, row++, 1);

  gui_layout->addWidget(snapping_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(snapping_line, row++, 1, Qt::AlignLeft);

  gui_layout->addWidget(stem_width_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(stem_width_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(default_stem_width_box, row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(wincomp_box, row++, 1);
  gui_layout->addWidget(pre_box, row++, 1);
  gui_layout->addWidget(hint_box, row++, 1);
  gui_layout->addWidget(symbol_box, row++, 1);
  gui_layout->addWidget(dehint_box, row++, 1);
  gui_layout->addWidget(info_box, row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(stem_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(gray_box, row++, 1);
  gui_layout->addWidget(gdi_box, row++, 1);
  gui_layout->addWidget(dw_box, row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(run_button, row++, 1, Qt::AlignRight);

  // create dummy widget to register layout
  QWidget* main_widget = new QWidget;
  main_widget->setLayout(gui_layout);
  setCentralWidget(main_widget);
  setWindowTitle("TTFautohint");
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
Main_GUI::create_horizontal_layout()
{
  // top area
  QGridLayout* file_layout = new QGridLayout;

  file_layout->addWidget(input_label, 0, 0, Qt::AlignRight);
  file_layout->addWidget(input_line, 0, 1);
  file_layout->addWidget(input_button, 0, 2);

  file_layout->setRowStretch(1, 1);

  file_layout->addWidget(output_label, 2, 0, Qt::AlignRight);
  file_layout->addWidget(output_line, 2, 1);
  file_layout->addWidget(output_button, 2, 2);

  QGridLayout* gui_layout = new QGridLayout;
  QFrame* hline = new QFrame;
  hline->setFrameShape(QFrame::HLine);
  int row = 0; // this counter simplifies inserting new items

  // margin
  gui_layout->setColumnMinimumWidth(0, 10); // XXX urgh, pixels...
  gui_layout->setColumnStretch(0, 1);

  // left
  gui_layout->setRowMinimumHeight(row, 10); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addLayout(file_layout, row, 0, row, -1);
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(hline, row, 0, row, -1);
  gui_layout->setRowStretch(row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(min_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(min_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(max_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(max_box, row++, 2, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(default_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(default_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(fallback_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(fallback_box, row++, 2, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(limit_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(limit_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(no_limit_box, row++, 2);

  gui_layout->addWidget(increase_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(increase_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(no_increase_box, row++, 2);

  gui_layout->addWidget(snapping_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(snapping_line, row++, 2, Qt::AlignLeft);

  gui_layout->addWidget(stem_width_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(stem_width_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(default_stem_width_box, row++, 2);

  // column separator
  gui_layout->setColumnMinimumWidth(3, 20); // XXX urgh, pixels...
  gui_layout->setColumnStretch(3, 1);

  // right
  row = 4;
  gui_layout->addWidget(wincomp_box, row++, 4);
  gui_layout->addWidget(pre_box, row++, 4);
  gui_layout->addWidget(hint_box, row++, 4);
  gui_layout->addWidget(symbol_box, row++, 4);
  gui_layout->addWidget(dehint_box, row++, 4);
  gui_layout->addWidget(info_box, row++, 4);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(stem_label, row++, 4);

  QGridLayout* stem_layout = new QGridLayout;
  stem_layout->setColumnMinimumWidth(0, 20); // XXX urgh, pixels...
  stem_layout->addWidget(gray_box, 0, 1);
  stem_layout->addWidget(gdi_box, 1, 1);
  stem_layout->addWidget(dw_box, 2, 1);

  gui_layout->addLayout(stem_layout, row, 4, 3, 1);
  row += 3;

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  row += 1;
  gui_layout->addWidget(run_button, row++, 4, Qt::AlignRight);

  // margin
  gui_layout->setColumnMinimumWidth(5, 10); // XXX urgh, pixels...
  gui_layout->setColumnStretch(5, 1);

  // create dummy widget to register layout
  QWidget* main_widget = new QWidget;
  main_widget->setLayout(gui_layout);
  setCentralWidget(main_widget);
  setWindowTitle("TTFautohint");
}


void
Main_GUI::create_connections()
{
  connect(input_button, SIGNAL(clicked()), this,
          SLOT(browse_input()));
  connect(output_button, SIGNAL(clicked()), this,
          SLOT(browse_output()));

  connect(input_line, SIGNAL(textChanged(QString)), this,
          SLOT(check_run()));
  connect(output_line, SIGNAL(textChanged(QString)), this,
          SLOT(check_run()));

  connect(input_line, SIGNAL(editingFinished()), this,
          SLOT(absolute_input()));
  connect(output_line, SIGNAL(editingFinished()), this,
          SLOT(absolute_output()));

  connect(min_box, SIGNAL(valueChanged(int)), this,
          SLOT(check_min()));
  connect(max_box, SIGNAL(valueChanged(int)), this,
          SLOT(check_max()));

  connect(limit_box, SIGNAL(valueChanged(int)), this,
          SLOT(check_limit()));
  connect(no_limit_box, SIGNAL(clicked()), this,
          SLOT(check_no_limit()));

  connect(no_increase_box, SIGNAL(clicked()), this,
          SLOT(check_no_increase()));

  connect(snapping_line, SIGNAL(editingFinished()), this,
          SLOT(check_number_set()));
  connect(snapping_line, SIGNAL(textEdited(QString)), this,
          SLOT(clear_status_bar()));

  connect(default_stem_width_box, SIGNAL(clicked()), this,
          SLOT(check_default_stem_width()));

  connect(dehint_box, SIGNAL(clicked()), this,
          SLOT(check_dehint()));

  connect(run_button, SIGNAL(clicked()), this,
          SLOT(run()));
}


void
Main_GUI::create_actions()
{
  exit_act = new QAction(tr("E&xit"), this);
  exit_act->setShortcuts(QKeySequence::Quit);
  connect(exit_act, SIGNAL(triggered()), this, SLOT(close()));

  about_act = new QAction(tr("&About"), this);
  connect(about_act, SIGNAL(triggered()), this, SLOT(about()));

  about_Qt_act = new QAction(tr("About &Qt"), this);
  connect(about_Qt_act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}


void
Main_GUI::create_menus()
{
  file_menu = menuBar()->addMenu(tr("&File"));
  file_menu->addAction(exit_act);

  help_menu = menuBar()->addMenu(tr("&Help"));
  help_menu->addAction(about_act);
  help_menu->addAction(about_Qt_act);
}


void
Main_GUI::create_status_bar()
{
  statusBar()->showMessage("");
}


void
Main_GUI::set_defaults()
{
  min_box->setValue(hinting_range_min);
  max_box->setValue(hinting_range_max);

  default_box->setCurrentIndex(default_script_idx);
  fallback_box->setCurrentIndex(fallback_script_idx);

  limit_box->setValue(hinting_limit ? hinting_limit : hinting_range_max);
  // handle command line option `--hinting-limit=0'
  if (!hinting_limit)
  {
    hinting_limit = max_box->value();
    no_limit_box->setChecked(true);
  }

  increase_box->setValue(increase_x_height ? increase_x_height
                                           : TA_INCREASE_X_HEIGHT);
  // handle command line option `--increase-x-height=0'
  if (!increase_x_height)
  {
    increase_x_height = TA_INCREASE_X_HEIGHT;
    no_increase_box->setChecked(true);
  }

  snapping_line->setText(x_height_snapping_exceptions_string);

  if (fallback_stem_width)
    stem_width_box->setValue(fallback_stem_width);
  else
  {
    stem_width_box->setValue(50);
    default_stem_width_box->setChecked(true);
  }

  if (windows_compatibility)
    wincomp_box->setChecked(true);
  if (pre_hinting)
    pre_box->setChecked(true);
  if (hint_composites)
    hint_box->setChecked(true);
  if (symbol)
    symbol_box->setChecked(true);
  if (dehint)
    dehint_box->setChecked(true);
  if (!no_info)
    info_box->setChecked(true);

  if (gray_strong_stem_width)
    gray_box->setChecked(true);
  if (gdi_cleartype_strong_stem_width)
    gdi_box->setChecked(true);
  if (dw_cleartype_strong_stem_width)
    dw_box->setChecked(true);

  run_button->setEnabled(false);

  check_min();
  check_max();
  check_limit();

  check_no_limit();
  check_no_increase();
  check_number_set();

  // do this last since it disables almost everything
  check_dehint();
}


void
Main_GUI::read_settings()
{
  QSettings settings;
//  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
//  QSize size = settings.value("size", QSize(400, 400)).toSize();
//  resize(size);
//  move(pos);
}


void
Main_GUI::write_settings()
{
  QSettings settings;
//  settings.setValue("pos", pos());
//  settings.setValue("size", size());
}

// end of maingui.cpp
