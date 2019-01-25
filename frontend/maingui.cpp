// maingui.cpp

// Copyright (C) 2012-2019 by Werner Lemberg.
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
#include <locale.h>

#include <QtGui>

#include <ft2build.h>
#include FT_FREETYPE_H

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


// Shorthand for `tr' using a local `TRDOMAIN'.
#define Tr(text) QCoreApplication::translate(TRDOMAIN, text)


typedef struct Tag_Names_
{
  const char* tag;
  const char* description;
} Tag_Names;


// the available script tags and its descriptions are directly extracted
// from `ttfautohint-scripts.h'
#undef SCRIPT
#define SCRIPT(s, S, d, h, H, ss) \
          {#s, d},

const Tag_Names script_names[] =
{
#include <ttfautohint-scripts.h>
  {NULL, NULL}
};


// the available feature tags and its descriptions are directly extracted
// from `ttfautohint-coverages.h'
#undef COVERAGE
#define COVERAGE(n, N, d, t, t1, t2, t3, t4) \
          {#t, d},

const Tag_Names feature_names[] =
{
#include <ttfautohint-coverages.h>
  {NULL, NULL}
};


// used hotkeys:
//   a: Add ttf&autohint Info
//   b: Add TTFA info ta&ble
//   c: x Height In&crease Limit / No x Height In&crease
//   d: &Dehint
//   e: Control Instructions Fil&e
//   f: &File (menu)
//   g: Stem Width and Positionin&g Mode
//   h: &Help (menu)
//   i: &Input Font
//   j: Ad&just Subglyphs
//   k: Fallbac&k Script
//   l: Hinting &Limit / No Hinting &Limit
//   m: Hint Co&mposites
//   n: Hint Set Range Mi&nimum
//   o: &Output Font
//   p: Windows Com&patibility
//   q: --
//   r: &Run
//   s: Fallback &Stem Width / Default Fallback &Stem Width
//   t: x Height Snapping Excep&tions
//   u: Defa&ult Script
//   v: --
//   w: &Watch Input Files
//   x: Hint Set Range Ma&ximum
//   y: S&ymbol Font
//   z: Blue &Zone Reference Font
//   :: Family Suffix&:

Main_GUI::Main_GUI(bool horizontal_layout,
                   int range_min,
                   int range_max,
                   int limit,
                   int gray,
                   int gdi,
                   int dw,
                   int increase,
                   const char* exceptions,
                   int stem_width,
                   bool ignore,
                   bool wincomp,
                   bool adjust,
                   bool composites,
                   bool no,
                   bool detailed,
                   const char* dflt,
                   const char* fallback,
                   bool scaling,
                   const char* suffix,
                   bool symb,
                   bool dh,
                   bool TTFA)
: hinting_range_min(range_min),
  hinting_range_max(range_max),
  hinting_limit(limit),
  gray_stem_width_mode(gray),
  gdi_cleartype_stem_width_mode(gdi),
  dw_cleartype_stem_width_mode(dw),
  increase_x_height(increase),
  x_height_snapping_exceptions_string(exceptions),
  fallback_stem_width(stem_width),
  ignore_restrictions(ignore),
  windows_compatibility(wincomp),
  adjust_subglyphs(adjust),
  hint_composites(composites),
  no_info(no),
  detailed_info(detailed),
  fallback_scaling(scaling),
  family_suffix(suffix),
  symbol(symb),
  dehint(dh),
  TTFA_info(TTFA),
  // constants
  fallback_do_scale(1),
  fallback_do_hint(0)
{
  int i;

  for (i = 0; script_names[i].tag; i++)
  {
    if (!strcmp("latn", script_names[i].tag))
      latn_script_idx = i;
    if (!strcmp("none", script_names[i].tag))
      none_script_idx = i;
  }

  // map default script and fallback script tags to indices
  for (i = 0; script_names[i].tag; i++)
    if (!strcmp(dflt, script_names[i].tag))
      break;
  default_script_idx = script_names[i].tag ? i : latn_script_idx;

  for (i = 0; script_names[i].tag; i++)
    if (!strcmp(fallback, script_names[i].tag))
      break;
  fallback_script_idx = script_names[i].tag ? i : none_script_idx;

  x_height_snapping_exceptions = NULL;

  // if the current input files have been updated
  // we wait a given time interval, then we reload the files
  file_watcher = new QFileSystemWatcher(this);
  timer = new QTimer(this);
  timer->setInterval(1000); // XXX make this configurable
  timer->setSingleShot(true);
  fileinfo_input_file.setCaching(false);
  fileinfo_control_file.setCaching(false);
  fileinfo_reference_file.setCaching(false);

  // XXX register translations somewhere and loop over them
  if (QLocale::system().name() == "en_US")
    locale = new QLocale;
  else
    locale = new QLocale(QLocale::C);

  // For real numbers (both parsing and displaying) we only use `.' as the
  // decimal separator; similarly, we don't want localized formats like a
  // thousands separator for any number.
  setlocale(LC_NUMERIC, "C");

  create_layout(horizontal_layout);
  create_connections();
  create_actions();
  create_menus();
  create_status_bar();

  set_defaults();
  read_settings();

  setUnifiedTitleAndToolBarOnMac(true);
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
                        " Copyright %2 2011-2019<br>"
                        " by Werner Lemberg <tt>&lt;wl@gnu.org&gt;</tt></p>"
                        ""
                        "<p><b>TTFautohint</b> adds new auto-generated hints"
                        " to a TrueType font or TrueType collection.</p>"
                        ""
                        "<p>License:"
                        " <a href='https://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/FTL.TXT'>FreeType"
                        " License (FTL)</a> or"
                        " <a href='https://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/GPLv2.TXT'>GNU"
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
                   tr("Open Input Font"),
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
                   tr("Open Output Font"),
                   QDir::homePath(),
                   "");

  if (!file.isEmpty())
    output_line->setText(QDir::toNativeSeparators(file));
}


void
Main_GUI::browse_control()
{
  // XXX remember last directory
  QString file = QFileDialog::getOpenFileName(
                   this,
                   tr("Open Control Instructions File"),
                   QDir::homePath(),
                   "");

  if (!file.isEmpty())
    control_line->setText(QDir::toNativeSeparators(file));
}


void
Main_GUI::browse_reference()
{
  // XXX remember last directory
  QString file = QFileDialog::getOpenFileName(
                   this,
                   tr("Open Blue Zone Reference Font"),
                   QDir::homePath(),
                   "");

  if (!file.isEmpty())
    reference_line->setText(QDir::toNativeSeparators(file));
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
    control_label->setEnabled(false);
    control_line->setEnabled(false);
    control_button->setEnabled(false);

    reference_label->setEnabled(false);
    reference_line->setEnabled(false);
    reference_button->setEnabled(false);

    min_label->setEnabled(false);
    min_box->setEnabled(false);

    max_label->setEnabled(false);
    max_box->setEnabled(false);

    default_label->setEnabled(false);
    default_box->setEnabled(false);
    fallback_label->setEnabled(false);
    fallback_hint_or_scale_box->setEnabled(false);
    fallback_box->setEnabled(false);

    no_limit_box->setEnabled(false);
    limit_label->setEnabled(false);
    limit_box->setEnabled(false);

    no_x_increase_box->setEnabled(false);
    x_increase_label->setEnabled(false);
    x_increase_box->setEnabled(false);

    snapping_label->setEnabled(false);
    snapping_line->setEnabled(false);

    default_stem_width_box->setEnabled(false);
    stem_width_label->setEnabled(false);
    stem_width_box->setEnabled(false);

    wincomp_box->setEnabled(false);
    adjust_box->setEnabled(false);
    hint_box->setEnabled(false);
    symbol_box->setEnabled(false);

    ref_idx_label->setEnabled(false);
    ref_idx_box->setEnabled(false);

    stem_label->setEnabled(false);

    gray_label->setEnabled(false);
    gray_box->setEnabled(false);
    gdi_label->setEnabled(false);
    gdi_box->setEnabled(false);
    dw_label->setEnabled(false);
    dw_box->setEnabled(false);
  }
  else
  {
    control_label->setEnabled(true);
    control_line->setEnabled(true);
    control_button->setEnabled(true);

    reference_label->setEnabled(true);
    reference_line->setEnabled(true);
    reference_button->setEnabled(true);

    min_label->setEnabled(true);
    min_box->setEnabled(true);

    max_label->setEnabled(true);
    max_box->setEnabled(true);

    default_label->setEnabled(true);
    default_box->setEnabled(true);
    fallback_label->setEnabled(true);
    fallback_hint_or_scale_box->setEnabled(true);
    fallback_box->setEnabled(true);

    no_limit_box->setEnabled(true);
    check_no_limit();

    no_x_increase_box->setEnabled(true);
    check_no_x_increase();

    snapping_label->setEnabled(true);
    snapping_line->setEnabled(true);

    default_stem_width_box->setEnabled(true);
    check_default_stem_width();

    wincomp_box->setEnabled(true);
    adjust_box->setEnabled(true);
    hint_box->setEnabled(true);
    symbol_box->setEnabled(true);

    ref_idx_label->setEnabled(true);
    ref_idx_box->setEnabled(true);

    stem_label->setEnabled(true);

    gray_label->setEnabled(true);
    gray_box->setEnabled(true);
    gdi_label->setEnabled(true);
    gdi_box->setEnabled(true);
    dw_label->setEnabled(true);
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
Main_GUI::check_no_x_increase()
{
  if (no_x_increase_box->isChecked())
  {
    x_increase_label->setEnabled(false);
    x_increase_label->setText(x_increase_label_text);
    x_increase_box->setEnabled(false);
    no_x_increase_box->setText(no_x_increase_box_text_with_key);
  }
  else
  {
    x_increase_label->setEnabled(true);
    x_increase_label->setText(x_increase_label_text_with_key);
    x_increase_box->setEnabled(true);
    no_x_increase_box->setText(no_x_increase_box_text);
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
Main_GUI::absolute_control()
{
  QString control_name = QDir::fromNativeSeparators(control_line->text());
  if (!control_name.isEmpty()
      && QDir::isRelativePath(control_name))
  {
    QDir cur_path(QDir::currentPath() + "/" + control_name);
    control_line->setText(QDir::toNativeSeparators(cur_path.absolutePath()));
  }
}


void
Main_GUI::absolute_reference()
{
  QString reference_name = QDir::fromNativeSeparators(reference_line->text());
  if (!reference_name.isEmpty()
      && QDir::isRelativePath(reference_name))
  {
    QDir cur_path(QDir::currentPath() + "/" + reference_name);
    reference_line->setText(QDir::toNativeSeparators(cur_path.absolutePath()));
  }
}


void
Main_GUI::set_ref_idx_box_max()
{
  QString reference_name = QDir::fromNativeSeparators(reference_line->text());

  FT_Library library = NULL;
  FT_Face face = NULL;
  FT_Error error;

  // we don't handle errors
  // since `TTF_autohint' checks the reference font (and index) later on
  error = FT_Init_FreeType(&library);
  if (!error)
    error = FT_New_Face(library, qPrintable(reference_name), -1, &face);

  ref_idx_box->setRange(0, error ? 100 : int(face->num_faces - 1));

  FT_Done_Face(face);
  FT_Done_FreeType(library);
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
    snapping_line->setCursorPosition(int(s - str.constData()));

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
Main_GUI::check_family_suffix()
{
  QByteArray text = family_suffix_line->text().toLocal8Bit();
  const char* s = text.constData();

  if (const char* pos = ::check_family_suffix(s))
  {
    statusBar()->setStyleSheet("color: red;");
    statusBar()->showMessage(
      tr("invalid character (use printable ASCII except %()/<>[]{})"));
    family_suffix_line->setFocus(Qt::OtherFocusReason);
    family_suffix_line->setCursorPosition(int(pos - s));
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
                          const QString& output_name,
                          const QString& control_name,
                          const QString& reference_name)
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

  // silently overwrite if watching is active
  if (QFile::exists(output_name) && !watch_box->isChecked())
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

  if (!control_name.isEmpty() && !QFile::exists(control_name))
  {
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The file %1 cannot be found.")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(control_name))),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  if (!reference_name.isEmpty() && !QFile::exists(reference_name))
  {
    QMessageBox::warning(
      this,
      "TTFautohint",
      tr("The file %1 cannot be found.")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(reference_name))),
      QMessageBox::Ok,
      QMessageBox::Ok);
    return 0;
  }

  return 1;
}


int
Main_GUI::open_files(const QString& input_name,
                     FILE** in,
                     const QString& output_name,
                     FILE** out,
                     const QString& control_name,
                     FILE** control,
                     const QString& reference_name,
                     FILE** reference)
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

  if (!control_name.isEmpty())
  {
    *control = fopen(qPrintable(control_name), "r");
    if (!*control)
    {
      strerror_r(errno, buf, buf_len);
      QMessageBox::warning(
        this,
        "TTFautohint",
        tr("The following error occurred"
           " while opening control instructions file %1:\n")
           .arg(QUOTE_STRING(QDir::toNativeSeparators(control_name)))
          + QString::fromLocal8Bit(buf),
        QMessageBox::Ok,
        QMessageBox::Ok);
      return 0;
    }
  }
  else
    *control = NULL;

  if (!reference_name.isEmpty())
  {
    *reference = fopen(qPrintable(reference_name), "rb");
    if (!*reference)
    {
      strerror_r(errno, buf, buf_len);
      QMessageBox::warning(
        this,
        "TTFautohint",
        tr("The following error occurred"
           " while opening blue zone reference file %1:\n")
           .arg(QUOTE_STRING(QDir::toNativeSeparators(reference_name)))
          + QString::fromLocal8Bit(buf),
        QMessageBox::Ok,
        QMessageBox::Ok);
      return 0;
    }
  }
  else
    *reference = NULL;

  return 1;
}


void
Main_GUI::check_watch()
{
  if (watch_box->isChecked())
  {
    // file watching gets started in the `run' function
    check = DoCheck;
  }
  else
    stop_watching();
}


void
Main_GUI::start_timer()
{
  // we delay the file watching action, mainly to ensure
  // that newly generated files have been completely written to disk
  check = CheckNow;
  timer->start();
}


void
Main_GUI::stop_watching()
{
  check = DoCheck;
  if (!fileinfo_input_file.fileName().isEmpty())
    file_watcher->removePath(fileinfo_input_file.fileName());
  if (!fileinfo_control_file.fileName().isEmpty())
    file_watcher->removePath(fileinfo_control_file.fileName());
  if (!fileinfo_reference_file.fileName().isEmpty())
    file_watcher->removePath(fileinfo_reference_file.fileName());
}


void
Main_GUI::watch_files()
{
  if (fileinfo_input_file.exists()
      && fileinfo_input_file.isReadable()
      && (fileinfo_control_file.fileName().isEmpty()
          ? true
          : (fileinfo_control_file.exists()
             && fileinfo_control_file.isReadable()))
      && (fileinfo_reference_file.fileName().isEmpty()
          ? true
          : (fileinfo_reference_file.exists()
             && fileinfo_reference_file.isReadable())))
  {
    QDateTime modified_input = fileinfo_input_file.lastModified();
    QDateTime modified_control = fileinfo_control_file.lastModified();
    QDateTime modified_reference = fileinfo_reference_file.lastModified();
    // XXX make this configurable
    if (datetime_input_file.msecsTo(modified_input) > 1000
        || datetime_control_file.msecsTo(modified_control) > 1000
        || datetime_reference_file.msecsTo(modified_reference) > 1000)
    {
      check = CheckNow;
      run(); // this function sets `datetime_XXX'
    }
    else
    {
      // restart timer for symlinks
      if (watch_box->isChecked())
      {
        if (fileinfo_input_file.isSymLink()
            || fileinfo_control_file.isSymLink()
            || fileinfo_reference_file.isSymLink())
          timer->start();
      }
    }
  }
  else
  {
    if (check == DoCheck)
      check = CheckLater;
    else if (check == CheckLater)
      check = CheckNow;

    run(); // let this routine handle all errors
  }
}


extern "C" {

struct GUI_Progress_Data
{
  long last_sfnt;
  bool begin;
  QProgressDialog* dialog;
};


#undef TRDOMAIN
#define TRDOMAIN "GuiProgress"

static int
gui_progress(long curr_idx,
             long num_glyphs,
             long curr_sfnt,
             long num_sfnts,
             void* user)
{
  GUI_Progress_Data* data = static_cast<GUI_Progress_Data*>(user);

  if (num_sfnts > 1 && curr_sfnt != data->last_sfnt)
  {
    data->dialog->setLabelText(Tr("Auto-hinting subfont %1 of %2"
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
      data->dialog->setLabelText(Tr("Auto-hinting %1 glyphs...")
                                    .arg(num_glyphs));
    data->dialog->setMaximum(int(num_glyphs - 1));

    data->begin = false;
  }

  data->dialog->setValue(int(curr_idx));

  if (data->dialog->wasCanceled())
    return 1;

  return 0;
}


struct GUI_Error_Data
{
  Main_GUI* gui;
  QLocale* locale;
  QString output_name;
  QString control_name;
  QString reference_name;
  int* ignore_restrictions_p;
  bool retry;
};


#undef TRDOMAIN
#define TRDOMAIN "GuiError"

static void
gui_error(TA_Error error,
          const char* error_string,
          unsigned int errlinenum,
          const char* errline,
          const char* errpos,
          void* user)
{
  GUI_Error_Data* data = static_cast<GUI_Error_Data*>(user);
  QLocale* locale = data->locale; // for QUOTE_STRING_LITERAL

  if (!error)
    return;

  if (error == TA_Err_Canceled)
    ;
  else if (error == TA_Err_Invalid_FreeType_Version)
    QMessageBox::critical(
      data->gui,
      "TTFautohint",
      Tr("FreeType version 2.4.5 or higher is needed.\n"
         "Are you perhaps using a wrong FreeType DLL?"),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Invalid_Font_Type)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("This font is not a valid font"
         " in SFNT format with TrueType outlines.\n"
         "In particular, CFF outlines are not supported."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Already_Processed)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("This font has already been processed by TTFautohint."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Legal_Permission)
  {
    int yesno = QMessageBox::warning(
                  data->gui,
                  "TTFautohint",
                  Tr("Bit 1 in the %1 field of the %2 table is set:"
                     " This font must not be modified"
                     " without permission of the legal owner.\n"
                     "Do you have such a permission?")
                     .arg(QUOTE_STRING_LITERAL("fsType"))
                     .arg(QUOTE_STRING_LITERAL("OS/2")),
                  QMessageBox::Yes | QMessageBox::No,
                  QMessageBox::No);
    if (yesno == QMessageBox::Yes)
    {
      *data->ignore_restrictions_p = true;
      data->retry = true;
    }
  }
  else if (error == TA_Err_Missing_Unicode_CMap)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("The input font doesn't contain a Unicode character map.\n"
         "Maybe you haven't set the %1 checkbox?")
         .arg(QUOTE_STRING_LITERAL(Tr("Symbol Font"))),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Symbol_CMap)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("The input font does neither contain a symbol"
         " nor a character map."),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error == TA_Err_Missing_Glyph)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("No glyph for a standard character"
         " to derive standard width and height.\n"
         "Please check the documentation for a list of"
         " script-specific standard characters.\n"
         "\n"
         "Set the %1 checkbox if you want to circumvent this test.")
         .arg(QUOTE_STRING_LITERAL(Tr("Symbol Font"))),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error >= 0x200 && error < 0x300)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      QString::fromLocal8Bit("%1:%2:%3: %4 (0x%5)<br>"
                             "<tt>  %6<br>"
                             "  %7</tt>")
                             .arg(data->control_name)
                             .arg(errlinenum)
                             .arg(int(errpos - errline + 1))
                             .arg(error_string)
                             .arg(error, 2, 16, QLatin1Char('0'))
                             .arg(errline)
                             .arg('^', int(errpos - errline + 1))
                             .replace(" ", "&nbsp;"),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else if (error >= 0x300 && error < 0x400)
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("Error code 0x%1 while loading blue zone reference file:\n")
         .arg(error - 0x300, 2, 16, QLatin1Char('0'))
        + QString::fromLocal8Bit(error_string),
      QMessageBox::Ok,
      QMessageBox::Ok);
  else
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("Error code 0x%1 while autohinting font:\n")
         .arg(error, 2, 16, QLatin1Char('0'))
        + QString::fromLocal8Bit(error_string),
      QMessageBox::Ok,
      QMessageBox::Ok);

  if (QFile::exists(data->output_name)
      && remove(qPrintable(data->output_name)))
  {
    const int buf_len = 1024;
    char buf[buf_len];

    strerror_r(errno, buf, buf_len);
    QMessageBox::warning(
      data->gui,
      "TTFautohint",
      Tr("The following error occurred while removing output file %1:\n")
         .arg(QUOTE_STRING(QDir::toNativeSeparators(data->output_name)))
        + QString::fromLocal8Bit(buf),
      QMessageBox::Ok,
      QMessageBox::Ok);
  }
}

} // extern "C"


void
Main_GUI::run()
{
  clear_status_bar();

  if (check == CheckLater)
  {
    timer->start();
    return;
  }

  QString input_name = QDir::fromNativeSeparators(input_line->text());
  QString output_name = QDir::fromNativeSeparators(output_line->text());
  QString control_name = QDir::fromNativeSeparators(control_line->text());
  QString reference_name = QDir::fromNativeSeparators(reference_line->text());
  if (!check_filenames(input_name, output_name, control_name, reference_name))
  {
    stop_watching();
    return;
  }

  // we need C file descriptors for communication with TTF_autohint
  FILE* input;
  FILE* output;
  FILE* control;
  FILE* reference;

again:
  if (!open_files(input_name, &input,
                  output_name, &output,
                  control_name, &control,
                  reference_name, &reference))
  {
    stop_watching();
    return;
  }

  QProgressDialog dialog;
  dialog.setCancelButtonText(tr("Cancel"));
  dialog.setMinimumDuration(1000);
  dialog.setWindowModality(Qt::WindowModal);

  TA_Info_Func info_func = info;
  TA_Info_Post_Func info_post_func = info_post;
  GUI_Progress_Data gui_progress_data = {-1, true, &dialog};
  GUI_Error_Data gui_error_data = {this, locale,
                                   output_name, control_name, reference_name,
                                   &ignore_restrictions, false};

  fileinfo_input_file.setFile(input_name);
  fileinfo_control_file.setFile(control_name);
  fileinfo_reference_file.setFile(reference_name);

  // prepare C strings
  QByteArray ctrl_name = fileinfo_control_file.fileName().toLocal8Bit();
  QByteArray ref_name = fileinfo_reference_file.fileName().toLocal8Bit();
  QByteArray except_str = x_height_snapping_exceptions_string.toLocal8Bit();
  QByteArray fam_suff = family_suffix_line->text().toLocal8Bit();

  Info_Data info_data;

  info_data.info_string = NULL; // must be deallocated after use
  info_data.info_string_wide = NULL; // must be deallocated after use
  info_data.info_string_len = 0;
  info_data.info_string_wide_len = 0;

  info_data.control_name = ctrl_name.isEmpty() ? NULL
                                               : ctrl_name.constData();
  info_data.reference_name = ref_name.isEmpty() ? NULL
                                                : ref_name.constData();
  info_data.reference_index = ref_idx_box->value();

  info_data.hinting_range_min = min_box->value();
  info_data.hinting_range_max = max_box->value();
  info_data.hinting_limit = no_limit_box->isChecked()
                            ? 0
                            : limit_box->value();

  // indices 0, 1, 2 are mapped to values -1, 0, 1
  info_data.gray_stem_width_mode = gray_box->currentIndex() - 1;
  info_data.gdi_cleartype_stem_width_mode = gdi_box->currentIndex() - 1;
  info_data.dw_cleartype_stem_width_mode = dw_box->currentIndex() - 1;

  info_data.increase_x_height = no_x_increase_box->isChecked()
                                ? 0
                                : x_increase_box->value();
  info_data.x_height_snapping_exceptions_string = except_str.constData();

  info_data.family_suffix = fam_suff.constData();
  info_data.family_data_head = NULL;

  info_data.fallback_stem_width = default_stem_width_box->isChecked()
                                  ? 0
                                  : stem_width_box->value();

  info_data.windows_compatibility = wincomp_box->isChecked();
  info_data.adjust_subglyphs = adjust_box->isChecked();
  info_data.hint_composites = hint_box->isChecked();
  info_data.symbol = symbol_box->isChecked();
  info_data.fallback_scaling = fallback_hint_or_scale_box->currentIndex();
  info_data.no_info = info_box->currentIndex() == 0;
  info_data.detailed_info = info_box->currentIndex() == 2;
  info_data.dehint = dehint_box->isChecked();
  info_data.TTFA_info = TTFA_box->isChecked();

  strncpy(info_data.default_script,
          script_names[default_box->currentIndex()].tag,
          sizeof (info_data.default_script));
  strncpy(info_data.fallback_script,
          script_names[fallback_box->currentIndex()].tag,
          sizeof (info_data.fallback_script));

  if (info_box->currentIndex())
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

  if (!*info_data.family_suffix)
    info_post_func = NULL;

  if (info_data.symbol
      && info_data.fallback_stem_width
      && info_data.fallback_scaling)
    QMessageBox::information(
      this,
      "TTFautohint",
      tr("Setting a fallback stem width for a symbol font"
         " with fallback scaling only has no effect."),
      QMessageBox::Ok,
      QMessageBox::Ok);

  datetime_input_file = fileinfo_input_file.lastModified();
  datetime_control_file = fileinfo_control_file.lastModified();
  datetime_reference_file = fileinfo_reference_file.lastModified();

  QByteArray snapping_string = snapping_line->text().toLocal8Bit();

  TA_Error error =
    TTF_autohint("in-file, out-file, control-file, reference-file,"
                 "reference-index, reference-name,"
                 "hinting-range-min, hinting-range-max,"
                 "hinting-limit,"
                 "gray-stem-width-mode,"
                 "gdi-cleartype-stem-width-mode,"
                 "dw-cleartype-stem-width-mode,"
                 "progress-callback, progress-callback-data,"
                 "error-callback, error-callback-data,"
                 "info-callback, info-post-callback, info-callback-data,"
                 "ignore-restrictions,"
                 "windows-compatibility,"
                 "adjust-subglyphs,"
                 "hint-composites,"
                 "increase-x-height,"
                 "x-height-snapping-exceptions, fallback-stem-width,"
                 "default-script,"
                 "fallback-script, fallback-scaling,"
                 "symbol, dehint, TTFA-info",
                 input, output, control, reference,
                 info_data.reference_index, info_data.reference_name,
                 info_data.hinting_range_min, info_data.hinting_range_max,
                 info_data.hinting_limit,
                 info_data.gray_stem_width_mode,
                 info_data.gdi_cleartype_stem_width_mode,
                 info_data.dw_cleartype_stem_width_mode,
                 gui_progress, &gui_progress_data,
                 gui_error, &gui_error_data,
                 info_func, info_post_func, &info_data,
                 ignore_restrictions,
                 info_data.windows_compatibility,
                 info_data.adjust_subglyphs,
                 info_data.hint_composites,
                 info_data.increase_x_height,
                 snapping_string.constData(), info_data.fallback_stem_width,
                 info_data.default_script,
                 info_data.fallback_script, info_data.fallback_scaling,
                 info_data.symbol, info_data.dehint, info_data.TTFA_info);

  if (info_box->currentIndex())
  {
    free(info_data.info_string);
    free(info_data.info_string_wide);
  }

  fclose(input);
  fclose(output);
  if (control)
    fclose(control);
  if (reference)
    fclose(reference);

  if (error)
  {
    // retry if there is a user request to do so (handled in `gui_error')
    if (gui_error_data.retry)
      goto again;

    stop_watching();
  }
  else
  {
    statusBar()->showMessage(tr("Auto-hinting finished")
                             + " ("
                             + QDateTime::currentDateTime()
                               .toString(Qt::TextDate)
                             + ").");

    // we have successfully processed a file;
    // start file watching now if requested
    if (watch_box->isChecked())
    {
      check = DoCheck;

      // Qt's file watcher doesn't handle symlinks;
      // we thus fall back to polling
      if (fileinfo_input_file.isSymLink()
          || fileinfo_control_file.isSymLink()
          || fileinfo_reference_file.isSymLink())
        timer->start();
      else
      {
        file_watcher->addPath(input_name);
        file_watcher->addPath(control_name);
        file_watcher->addPath(reference_name);
      }
    }
  }
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

  input_label = new QLabel(tr("&Input Font:"));
  input_line = new Drag_Drop_Line_Edit(DRAG_DROP_TRUETYPE);
  input_button = new QPushButton(tr("Browse..."));
  input_label->setBuddy(input_line);
  // enforce rich text to get nice word wrapping
  input_label->setToolTip(
    tr("<b></b>The input file, either a TrueType font (TTF),"
       " TrueType collection (TTC), or a TrueType-based OpenType font."));
  input_line->setCompleter(completer);

  output_label = new QLabel(tr("&Output Font:"));
  output_line = new Drag_Drop_Line_Edit(DRAG_DROP_TRUETYPE);
  output_button = new QPushButton(tr("Browse..."));
  output_label->setBuddy(output_line);
  output_label->setToolTip(
    tr("<b></b>The output file, which will be essentially identical"
       " to the input font but will contain new, generated hints."));
  output_line->setCompleter(completer);

  control_label = new QLabel(tr("Control Instructions Fil&e:"));
  control_line = new Drag_Drop_Line_Edit(DRAG_DROP_ANY);
  control_button = new QPushButton(tr("Browse..."));
  control_label->setBuddy(control_line);
  // we use the non-breaking hyphen U+2011 (&#8209;) where necessary
  QString tooltip_string =
    tr("<p>An optional control instructions file to tweak hinting"
       " and to override glyph assignments to styles."
       "  This text file contains entries"
       " of one of the following syntax forms"
       " (with brackets indicating optional elements).<br>"
       "&nbsp;<br>"

       "&nbsp;&nbsp;[&nbsp;<i>subfont&#8209;idx</i>&nbsp;]"
       "&nbsp;&nbsp;<i>script</i>"
       "&nbsp;&nbsp;<i>feature</i>"
       "&nbsp;&nbsp;<tt>@</tt>&nbsp;<i>glyph&#8209;ids</i><br>"

       "&nbsp;&nbsp;[&nbsp;<i>subfont&#8209;idx</i>&nbsp;]"
       "&nbsp;&nbsp;<i>script</i>"
       "&nbsp;&nbsp;<i>feature</i>"
       "&nbsp;&nbsp;<tt>width</tt>&nbsp;<i>stem&#8209;widths</i><br>"

       "&nbsp;&nbsp;[&nbsp;<i>subfont&#8209;idx</i>&nbsp;]"
       "&nbsp;&nbsp;<i>glyph&#8209;id</i>"
       "&nbsp;&nbsp;<tt>left</tt>&nbsp;|&nbsp;<tt>right</tt>&nbsp;<i>points</i>"
       "&nbsp;&nbsp;[&nbsp;<tt>(</tt><i>left&#8209;offset</i><tt>,"
         "</tt><i>right&#8209;offset</i><tt>)</tt>&nbsp;]<br>"

       "&nbsp;&nbsp;[&nbsp;<i>subfont&#8209;idx</i>&nbsp;]"
       "&nbsp;&nbsp;<i>glyph&#8209;id</i>"
       "&nbsp;&nbsp;<tt>nodir</tt>&nbsp;<i>points</i><br>"

       "&nbsp;&nbsp;[&nbsp;<i>subfont&#8209;idx</i>&nbsp;]"
       "&nbsp;&nbsp;<i>glyph&#8209;id</i>"
       "&nbsp;&nbsp;<tt>touch</tt>&nbsp;|&nbsp;<tt>point</tt>&nbsp;<i>points</i>"
       "&nbsp;&nbsp;[&nbsp;<tt>xshift</tt>&nbsp;<i>shift</i>&nbsp;]"
       "&nbsp;&nbsp;[&nbsp;<tt>yshift</tt>&nbsp;<i>shift</i>&nbsp;]"
       "&nbsp;&nbsp;<tt>@</tt>&nbsp;<i>ppems</i><br>"
       "&nbsp;<br>"

       "<i>subfont&#8209;idx</i> gives the subfont index in a TTC,"
       " <i>glyph&#8209;id</i> is a glyph name or index.<br>"
       "&nbsp;<br>"

       "<i>script</i> and <i>feature</i> are four-letter tags"
       " (like <tt>cyrl</tt> for the Cyrillic script"
       " or <tt>subs</tt> for the subscript feature)"
       " that define a style the <i>glyph&#8209;ids</i> are assigned to."
       "  <i>glyph&#8209;ids</i> is a comma-separated list of"
       " <i>glyph&#8209;id</i> values and value ranges.<br>"
       "&nbsp;<br>"

       "<i>stem&#8209;widths</i> is a comma-separated list of integer"
       " stem widths (in font units); the first value sets the default stem"
       " width.<br>"
       "&nbsp;<br>"

       "<tt>left</tt> (<tt>right</tt>) creates one-point segments"
       " with direction left (right), possibly having a width (in font units)"
       " given by <i>left&#8209;offset</i> and <i>right&#8209;offset</i>"
       " relative to the corresponding points.<br>"
       "&nbsp;<br>"

       "<tt>nodir</tt> removes points from horizontal segments,"
       " making them <i>weak</i> points.<br>"
       "&nbsp;<br>"

       "<tt>touch </tt> (<tt>point</tt>) defines delta exceptions"
       " to be applied before (after) the final"
       " <tt>IUP</tt> bytecode instructions."
       "  <tt>touch</tt> also touches points, making them <i>strong</i>."
       "  In ClearType mode, <tt>point</tt> and <tt>xshift</tt>"
       " have no effect.<br>"
       "&nbsp;<br>"

       "x and y <i>shift</i> values are in the range [-1;1],"
       " rounded to multiples of 1/8px.<br>"

       "<i>points</i> and <i>ppems</i> are ranges for point indices"
       " and ppem values as with x&nbsp;height snapping exceptions.<br>"

       "Keywords <tt>left</tt>, <tt>right</tt>, <tt>nodir</tt>,"
       " <tt>point</tt>, <tt>touch</tt>, <tt>width</tt>, <tt>xshift</tt>,"
       " and <tt>yshift</tt> can be abbreviated as <tt>l</tt>, <tt>r</tt>,"
       " <tt>n</tt>, <tt>p</tt>, <tt>t</tt>, <tt>w</tt>, <tt>x</tt>, and"
       " <tt>y</tt>, respectively.<br>"

       "Control instruction entries are separated"
       " by character&nbsp;<tt>;</tt> or by a newline.<br>"

       "A trailing character&nbsp;<tt>\\</tt> continues the current line"
       " on the next line.<br>"

       "<tt>#</tt> starts a line comment, which gets ignored."
       "  Empty lines are ignored, too.</p>"

       "Examples:<br>"
       "&nbsp;&nbsp;<tt>cyrl sups @ one.sups-nine.sups, zero.sups</tt><br>"
       "&nbsp;&nbsp;<tt>Q left 38 (-70,20)</tt><br>"
       "&nbsp;&nbsp;<tt>Adieresis touch 3-6 yshift 0.25 @ 13</tt>");

  control_label->setToolTip(tooltip_string);
  control_line->setCompleter(completer);

  reference_label = new QLabel(tr("Blue &Zone Reference Font:"));
  reference_line = new Drag_Drop_Line_Edit(DRAG_DROP_TRUETYPE);
  reference_button = new QPushButton(tr("Browse..."));
  reference_label->setBuddy(reference_line);
  reference_label->setToolTip(
    tr("<b></b>A reference font file"
       " from which all blue zone values are taken."));
  reference_line->setCompleter(completer);

  ref_idx_label = new QLabel(tr("Reference Face Index:"));
  ref_idx_box = new QSpinBox;
  ref_idx_label->setBuddy(ref_idx_box);
  ref_idx_label->setToolTip(
    tr("The face index of the blue zone reference font to be used"
       " in case it is a TrueType Collection (<tt>.ttc</tt>)."));
  ref_idx_box->setKeyboardTracking(false);
  // the range maximum gets updated interactively
  // after a (more or less) valid reference font has been selected
  ref_idx_box->setRange(0, 100);

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
       " <b>TTFautohint</b> uses this value for the remaining features."
       ""
       "<p>In most cases, you don't want to change this value.</p>"));
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
  fallback_label = new QLabel(tr("Fallbac&k Script"));
  fallback_hint_or_scale_box = new QComboBox;
  fallback_box = new QComboBox;
  fallback_label->setBuddy(fallback_hint_or_scale_box);
  fallback_label->setToolTip(
    tr("This sets the fallback script for glyphs"
       " that <b>TTFautohint</b> can't map to a script automatically."
       "  Either hinting with the script's blue zone values"
       " or simple scaling with the script's scaling value"
       " can be selected."));
  fallback_hint_or_scale_box->insertItem(fallback_do_scale, tr("Scaling:"));
  fallback_hint_or_scale_box->insertItem(fallback_do_hint, tr("Hinting:"));
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
    tr("Switch off hinting for PPEM values exceeding this limit."
       " Changing this value does not influence the size of the bytecode.<br>"
       "Note that <b>TTFautohint</b> handles this feature"
       " in the output font's bytecode and not in the <i>gasp</i> table."));
  limit_box->setKeyboardTracking(false);
  limit_box->setRange(2, 10000);

  no_limit_box_text_with_key = tr("No Hinting &Limit");
  no_limit_box_text = tr("No Hinting Limit");
  no_limit_box = new QCheckBox(no_limit_box_text);
  no_limit_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> adds no hinting limit"
       " to the bytecode.<br>"
       "For testing only."));

  //
  // x height increase limit
  //
  x_increase_label_text_with_key = tr("x Height In&crease Limit:");
  x_increase_label_text = tr("x Height Increase Limit:");
  x_increase_label = new QLabel(x_increase_label_text_with_key);
  x_increase_box = new QSpinBox;
  x_increase_label->setBuddy(x_increase_box);
  x_increase_label->setToolTip(
    tr("For PPEM values in the range 6&nbsp;&le; PPEM &le;&nbsp;<i>n</i>,"
       " where <i>n</i> is the value selected by this spin box,"
       " round up the font's x&nbsp;height much more often than normally.<br>"
       "Use this if holes in letters like <i>e</i> get filled,"
       " for example."));
  x_increase_box->setKeyboardTracking(false);
  x_increase_box->setRange(6, 10000);

  no_x_increase_box_text_with_key = tr("No x Height In&crease");
  no_x_increase_box_text = tr("No x Height Increase");
  no_x_increase_box = new QCheckBox(no_x_increase_box_text);
  no_x_increase_box->setToolTip(
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
  // family suffix
  //
  family_suffix_label = new QLabel(tr("Family Suffix&:"));
  family_suffix_line = new Tooltip_Line_Edit;
  family_suffix_label->setBuddy(family_suffix_line);
  family_suffix_label->setToolTip(
    tr("A string that gets appended to the family name entries"
       " (name IDs 1, 4, 6, 16, and&nbsp;21)"
       " in the font's <i>name</i> table.<br>"
       "Mainly for testing purposes, enabling the operating system"
       " to display fonts simultaneously that are hinted"
       " with different <b>TTFautohint</b> parameters."));

  //
  // fallback stem width
  //
  stem_width_label_text_with_key = tr("Fallback &Stem Width:");
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

  default_stem_width_box_text_with_key = tr("Default Fallback &Stem Width");
  default_stem_width_box_text = tr("Default Fallback Stem Width");
  default_stem_width_box = new QCheckBox(default_stem_width_box_text);
  default_stem_width_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> uses a default value"
       " for the fallback stem width (50 font units at 2048 UPEM)."));

  //
  // flags
  //
  wincomp_box = new QCheckBox(tr("Windows Com&patibility"));
  wincomp_box->setToolTip(
    tr("If switched on, add two artificial blue zones positioned at the"
       " <tt>usWinAscent</tt> and <tt>usWinDescent</tt> values"
       " (from the font's <i>OS/2</i> table)."
       "  This option, usually in combination"
       " with value <tt>-</tt> (a single dash)"
       " for the <i>x&nbsp;Height Snapping Exceptions</i> option,"
       " should be used if those two <i>OS/2</i> values are tight,"
       " and you are experiencing clipping during rendering."));

  adjust_box = new QCheckBox(tr("Ad&just Subglyphs"));
  adjust_box->setToolTip(
    tr("If switched on, the original bytecode of the input font"
       " gets applied (at EM size, usually 2048ppem)"
       " to derive the glyph outlines for <b>TTFautohint</b>.<br>"
       "Use this option only if subglyphs"
       " are incorrectly scaled and shifted.<br>"
       "Note that the original bytecode will always be discarded."));

  hint_box = new QCheckBox(tr("Hint Co&mposites")
                           + "                "); // make label wider
  hint_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> hints composite glyphs"
       " as a whole, including subglyphs."
       "  Otherwise, glyph components get hinted separately.<br>"
       "Deactivating this flag reduces the bytecode size enormously,"
       " however, it might yield worse results."));

  symbol_box = new QCheckBox(tr("S&ymbol Font"));
  symbol_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> accepts fonts"
       " that don't contain a single standard character"
       " for any of the supported scripts.<br>"
       "Use this for symbol or dingbat fonts, for example."));

  dehint_box = new QCheckBox(tr("&Dehint"));
  dehint_box->setToolTip(
    tr("If set, remove all hints from the font.<br>"
       "For testing only."));

  info_label = new QLabel(tr("ttf&autohint Info:"));
  info_box = new QComboBox;
  info_label->setBuddy(info_box);
  info_label->setToolTip(
    tr("Specify which information about <b>TTFautohint</b>"
       " and its calling parameters gets added to the version string(s)"
       " (name ID&nbsp;5) in the <i>name</i> table.<br>"
       "If you want to archive control instruction information,"
       " use a <i>TTFA</i> table."));
  info_box->insertItem(0, tr("None"));
  info_box->insertItem(1, tr("Version"));
  info_box->insertItem(2, tr("Version and Parameters"));

  TTFA_box = new QCheckBox(tr("Add TTFA Info Ta&ble"));
  TTFA_box->setToolTip(
    tr("If switched on, an SFNT table called <i>TTFA</i>"
       " gets added to the output font,"
       " holding a dump of all parameters."
       "  In particular, it lists all control instructions."));

  //
  // stem width and positioning
  //
  stem_label = new QLabel(tr("Stem Width and Positionin&g Mode"));
  stem_label->setToolTip(
    tr("<b>TTFautohint</b> provides three different hinting algorithms"
       " that can be selected for various hinting modes."
       ""
       "<p><i>strong</i>:"
       " Position horizontal stems and snap stem widths"
       " to integer pixel values.  While making the output look crisper,"
       " outlines become more distorted.</p>"
       ""
       "<p><i>quantized</i>:"
       " Use discrete values for horizontal stems and stem widths."
       "  This only slightly increases the contrast"
       " but avoids larger outline distortion.</p>"
       ""
       "<p><i>natural</i>:"
       " Don't change horizontal stem widths"
       " but use discrete values for stem positioning."
       " This is what FreeType's"
       " &lsquo;light&rsquo; hinting mode does.</p>"));

  QStringList mode_list = QStringList() << "natural"
                                        << "quantized"
                                        << "strong";

  gray_label = new QLabel(tr("Grayscale:"));
  gray_box = new QComboBox;
  gray_box->insertItems(0, mode_list);
  gray_label->setToolTip(
    tr("<b></b>Grayscale rendering, no ClearType activated."));

  // this must come after the creation of `gray_box'
  // (you get a bus error otherwise)
  stem_label->setBuddy(gray_box);

  gdi_label = new QLabel(tr("GDI ClearType:"));
  gdi_box = new QComboBox;
  gdi_box->insertItems(0, mode_list);
  gdi_label->setToolTip(
    tr("GDI ClearType rendering,"
       " introduced in 2000 for Windows XP.<br>"
       "The rasterizer version (as returned by the"
       " GETINFO bytecode instruction) is in the range"
       " 36&nbsp;&le; version &lt;&nbsp;38, and ClearType is enabled.<br>"
       "Along the vertical axis, this mode behaves like B/W rendering."));

  dw_label = new QLabel(tr("DW ClearType:"));
  dw_box = new QComboBox;
  dw_box->insertItems(0, mode_list);
  dw_label->setToolTip(
    tr("DirectWrite ClearType rendering,"
       " introduced in 2008 for Windows Vista.<br>"
       "The rasterizer version (as returned by the"
       " GETINFO bytecode instruction) is &ge;&nbsp;38,"
       " ClearType is enabled, and subpixel positioning is enabled also.<br>"
       "Smooth rendering along the vertical axis."));

  //
  // running
  //
  watch_box = new QCheckBox(tr("&Watch Input Files"));
  watch_box->setToolTip(
    tr("If switched on, <b>TTFautohint</b> automatically re-runs"
       " the hinting process as soon as an input file"
       " (either the font, the reference font,"
       " or the control instructions file) is modified.<br>"
       "Pressing the %1 button starts watching.<br>"
       "If an error occurs, watching stops and must be restarted"
       " with the %1 button.")
       .arg(QUOTE_STRING_LITERAL(tr("Run"))));

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

  file_layout->setRowStretch(3, 1);

  file_layout->addWidget(control_label, 4, 0, Qt::AlignRight);
  file_layout->addWidget(control_line, 4, 1);
  file_layout->addWidget(control_button, 4, 2);

  file_layout->setRowStretch(5, 1);

  file_layout->addWidget(reference_label, 6, 0, Qt::AlignRight);
  file_layout->addWidget(reference_line, 6, 1);
  file_layout->addWidget(reference_button, 6, 2);

  // bottom area
  QGridLayout* run_layout = new QGridLayout;

  run_layout->addWidget(watch_box, 0, 1);
  run_layout->addWidget(run_button, 0, 3, Qt::AlignRight);
  run_layout->setColumnStretch(0, 1);
  run_layout->setColumnStretch(2, 2);

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

  QGridLayout* fallback_layout = new QGridLayout;
  fallback_layout->addWidget(fallback_label, 0, 0, Qt::AlignRight);
  fallback_layout->addWidget(fallback_hint_or_scale_box, 0, 1, Qt::AlignLeft);

  gui_layout->addLayout(fallback_layout, row++, 0, Qt::AlignRight);
  gui_layout->addWidget(fallback_box, row++, 1, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(limit_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(limit_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(no_limit_box, row++, 1);

  gui_layout->addWidget(x_increase_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(x_increase_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(no_x_increase_box, row++, 1);

  gui_layout->addWidget(snapping_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(snapping_line, row++, 1, Qt::AlignLeft);

  gui_layout->addWidget(stem_width_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(stem_width_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(default_stem_width_box, row++, 1);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(wincomp_box, row++, 1);
  gui_layout->addWidget(adjust_box, row++, 1);
  gui_layout->addWidget(hint_box, row++, 1);
  gui_layout->addWidget(symbol_box, row++, 1);
  gui_layout->addWidget(dehint_box, row++, 1);

  gui_layout->addWidget(info_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(info_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(TTFA_box, row++, 1);
  gui_layout->addWidget(family_suffix_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(family_suffix_line, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(ref_idx_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(ref_idx_box, row++, 1, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  // this item spans two columns
  gui_layout->addWidget(stem_label, row++, 0, 1, 2, Qt::AlignCenter);

  gui_layout->addWidget(gray_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(gray_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(gdi_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(gdi_box, row++, 1, Qt::AlignLeft);
  gui_layout->addWidget(dw_label, row, 0, Qt::AlignRight);
  gui_layout->addWidget(dw_box, row++, 1, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 30); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addLayout(run_layout, row, 0, row, -1);

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

  file_layout->setRowStretch(3, 1);

  file_layout->addWidget(control_label, 4, 0, Qt::AlignRight);
  file_layout->addWidget(control_line, 4, 1);
  file_layout->addWidget(control_button, 4, 2);

  file_layout->setRowStretch(5, 1);

  file_layout->addWidget(reference_label, 6, 0, Qt::AlignRight);
  file_layout->addWidget(reference_line, 6, 1);
  file_layout->addWidget(reference_button, 6, 2);

  // bottom area
  QGridLayout* run_layout = new QGridLayout;

  run_layout->addWidget(watch_box, 0, 1);
  run_layout->addWidget(run_button, 0, 3, Qt::AlignRight);
  run_layout->setColumnStretch(0, 2);
  run_layout->setColumnStretch(2, 3);
  run_layout->setColumnStretch(4, 1);

  //
  // the whole gui
  //
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

  QGridLayout* fallback_layout = new QGridLayout;
  fallback_layout->addWidget(fallback_label, 0, 0, Qt::AlignRight);
  fallback_layout->addWidget(fallback_hint_or_scale_box, 0, 1, Qt::AlignLeft);

  gui_layout->addLayout(fallback_layout, row, 1, Qt::AlignRight);
  gui_layout->addWidget(fallback_box, row++, 2, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addWidget(limit_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(limit_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(no_limit_box, row++, 2);

  gui_layout->addWidget(x_increase_label, row, 1, Qt::AlignRight);
  gui_layout->addWidget(x_increase_box, row++, 2, Qt::AlignLeft);
  gui_layout->addWidget(no_x_increase_box, row++, 2);

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
  gui_layout->addWidget(adjust_box, row++, 4);
  gui_layout->addWidget(hint_box, row++, 4);
  gui_layout->addWidget(symbol_box, row++, 4);
  gui_layout->addWidget(dehint_box, row++, 4);

  gui_layout->addWidget(info_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(info_box, row++, 4, Qt::AlignLeft);
  gui_layout->addWidget(TTFA_box, row++, 4);
  gui_layout->addWidget(family_suffix_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(family_suffix_line, row++, 4, Qt::AlignLeft);
  gui_layout->addWidget(ref_idx_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(ref_idx_box, row++, 4, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 20); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  // this item spans two columns
  gui_layout->addWidget(stem_label, row++, 3, 1, 2, Qt::AlignCenter);

  gui_layout->addWidget(gray_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(gray_box, row++, 4, Qt::AlignLeft);
  gui_layout->addWidget(gdi_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(gdi_box, row++, 4, Qt::AlignLeft);
  gui_layout->addWidget(dw_label, row, 3, Qt::AlignRight);
  gui_layout->addWidget(dw_box, row++, 4, Qt::AlignLeft);

  gui_layout->setRowMinimumHeight(row, 30); // XXX urgh, pixels...
  gui_layout->setRowStretch(row++, 1);

  gui_layout->addLayout(run_layout, row, 0, row, -1);

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
  connect(input_button, SIGNAL(clicked()),
          SLOT(browse_input()));
  connect(output_button, SIGNAL(clicked()),
          SLOT(browse_output()));
  connect(control_button, SIGNAL(clicked()),
          SLOT(browse_control()));
  connect(reference_button, SIGNAL(clicked()),
          SLOT(browse_reference()));

  connect(input_line, SIGNAL(textChanged(QString)),
          SLOT(check_run()));
  connect(output_line, SIGNAL(textChanged(QString)),
          SLOT(check_run()));

  connect(input_line, SIGNAL(editingFinished()),
          SLOT(absolute_input()));
  connect(output_line, SIGNAL(editingFinished()),
          SLOT(absolute_output()));
  connect(control_line, SIGNAL(editingFinished()),
          SLOT(absolute_control()));
  connect(reference_line, SIGNAL(editingFinished()),
          SLOT(absolute_reference()));

  connect(reference_line, SIGNAL(editingFinished()),
          SLOT(set_ref_idx_box_max()));

  connect(min_box, SIGNAL(valueChanged(int)),
          SLOT(check_min()));
  connect(max_box, SIGNAL(valueChanged(int)),
          SLOT(check_max()));

  connect(limit_box, SIGNAL(valueChanged(int)),
          SLOT(check_limit()));
  connect(no_limit_box, SIGNAL(clicked()),
          SLOT(check_no_limit()));

  connect(no_x_increase_box, SIGNAL(clicked()),
          SLOT(check_no_x_increase()));

  connect(snapping_line, SIGNAL(editingFinished()),
          SLOT(check_number_set()));
  connect(snapping_line, SIGNAL(textEdited(QString)),
          SLOT(clear_status_bar()));

  connect(family_suffix_line, SIGNAL(editingFinished()),
          SLOT(check_family_suffix()));
  connect(family_suffix_line, SIGNAL(textEdited(QString)),
          SLOT(clear_status_bar()));

  connect(default_stem_width_box, SIGNAL(clicked()),
          SLOT(check_default_stem_width()));

  connect(dehint_box, SIGNAL(clicked()),
          SLOT(check_dehint()));

  connect(file_watcher, SIGNAL(fileChanged(const QString&)),
          SLOT(start_timer()));
  connect(timer, SIGNAL(timeout()),
          SLOT(watch_files()));

  connect(watch_box, SIGNAL(clicked()),
          SLOT(check_watch()));

  connect(run_button, SIGNAL(clicked()),
          SLOT(run()));
}


void
Main_GUI::create_actions()
{
  exit_act = new QAction(tr("E&xit"), this);
  exit_act->setShortcuts(QKeySequence::Quit);
  connect(exit_act, SIGNAL(triggered()), SLOT(close()));

  about_act = new QAction(tr("&About"), this);
  connect(about_act, SIGNAL(triggered()), SLOT(about()));

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
  fallback_hint_or_scale_box->setCurrentIndex(fallback_scaling);
  fallback_box->setCurrentIndex(fallback_script_idx);

  limit_box->setValue(hinting_limit ? hinting_limit : hinting_range_max);
  // handle command line option `--hinting-limit=0'
  if (!hinting_limit)
  {
    hinting_limit = max_box->value();
    no_limit_box->setChecked(true);
  }

  x_increase_box->setValue(increase_x_height ? increase_x_height
                                             : TA_INCREASE_X_HEIGHT);
  // handle command line option `--increase-x-height=0'
  if (!increase_x_height)
  {
    increase_x_height = TA_INCREASE_X_HEIGHT;
    no_x_increase_box->setChecked(true);
  }

  snapping_line->setText(x_height_snapping_exceptions_string);
  family_suffix_line->setText(family_suffix);

  if (fallback_stem_width)
    stem_width_box->setValue(fallback_stem_width);
  else
  {
    stem_width_box->setValue(50);
    default_stem_width_box->setChecked(true);
  }

  if (windows_compatibility)
    wincomp_box->setChecked(true);
  if (adjust_subglyphs)
    adjust_box->setChecked(true);
  if (hint_composites)
    hint_box->setChecked(true);
  if (symbol)
    symbol_box->setChecked(true);
  if (dehint)
    dehint_box->setChecked(true);
  if (no_info)
    info_box->setCurrentIndex(0);
  else if (detailed_info)
    info_box->setCurrentIndex(2);
  else
    info_box->setCurrentIndex(1);
  if (TTFA_info)
    TTFA_box->setChecked(true);

  gray_box->setCurrentIndex(gray_stem_width_mode + 1);
  gdi_box->setCurrentIndex(gdi_cleartype_stem_width_mode + 1);
  dw_box->setCurrentIndex(dw_cleartype_stem_width_mode + 1);

  run_button->setEnabled(false);

  check_min();
  check_max();
  check_limit();

  check_no_limit();
  check_no_x_increase();
  check_number_set();

  check_watch();

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
