// maingui.h

// Copyright (C) 2012-2019 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#ifndef MAINGUI_H_
#define MAINGUI_H_

#include <config.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>

#include "ddlineedit.h"
#include "ttlineedit.h"

#include <stdio.h>
#include <ttfautohint.h>
#include <numberset.h>

class QAction;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QFile;
class QLabel;
class QLocale;
class QMenu;
class QPushButton;
class QSpinBox;

class Drag_Drop_Line_Edit;
class Tooltip_Line_Edit;

class Main_GUI
: public QMainWindow
{
  Q_OBJECT

public:
  Main_GUI(bool,
           int, int, int,
           int, int,
           int, int,
           const char*, int,
           bool, bool, bool,
           bool, bool, bool,
           const char*, const char*, bool,
           const char*, bool, bool, bool);
  ~Main_GUI();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void about();
  void browse_input();
  void browse_output();
  void browse_control();
  void browse_reference();
  void check_min();
  void check_max();
  void check_limit();
  void check_dehint();
  void check_no_limit();
  void check_no_x_increase();
  void check_default_stem_width();
  void absolute_input();
  void absolute_output();
  void absolute_control();
  void absolute_reference();
  void set_ref_idx_box_max();
  void check_number_set();
  void check_family_suffix();
  void clear_status_bar();
  void start_timer();
  void check_watch();
  void watch_files();
  void check_run();
  void run();

private:
  int hinting_range_min;
  int hinting_range_max;
  int hinting_limit;
  int gray_stem_width_mode;
  int gdi_cleartype_stem_width_mode;
  int dw_cleartype_stem_width_mode;
  int increase_x_height;
  QString x_height_snapping_exceptions_string;
  number_range* x_height_snapping_exceptions;
  int fallback_stem_width;
  int ignore_restrictions;
  int windows_compatibility;
  int adjust_subglyphs;
  int hint_composites;
  int no_info;
  int detailed_info;
  int default_script_idx;
  int fallback_script_idx;
  int fallback_scaling;
  QString family_suffix;
  int reference_index;
  int symbol;
  int dehint;
  int TTFA_info;

  const int fallback_do_scale;
  const int fallback_do_hint;

  int latn_script_idx;
  int none_script_idx;

  void create_layout(bool);
  void create_horizontal_layout();
  void create_vertical_layout();

  enum CheckState {
    DoCheck,
    CheckNow,
    CheckLater
  };

  QFileSystemWatcher* file_watcher;
  QTimer* timer;
  QFileInfo fileinfo_input_file;
  QFileInfo fileinfo_control_file;
  QFileInfo fileinfo_reference_file;
  QDateTime datetime_input_file;
  QDateTime datetime_control_file;
  QDateTime datetime_reference_file;
  CheckState check;

  void create_connections();
  void create_actions();
  void create_menus();
  void create_status_bar();
  void set_defaults();
  void read_settings();
  void write_settings();

  int check_filenames(const QString&,
                      const QString&,
                      const QString&,
                      const QString&);
  int open_files(const QString&, FILE**,
                 const QString&, FILE**,
                 const QString&, FILE**,
                 const QString&, FILE**);
  int handle_error(TA_Error, const unsigned char*, QString);

  void stop_watching();

  QMenu* file_menu;
  QMenu* help_menu;

  QLabel* input_label;
  Drag_Drop_Line_Edit* input_line;
  QPushButton* input_button;

  QLabel* output_label;
  Drag_Drop_Line_Edit* output_line;
  QPushButton* output_button;

  QLabel* control_label;
  Drag_Drop_Line_Edit* control_line;
  QPushButton* control_button;

  QLabel* reference_label;
  Drag_Drop_Line_Edit* reference_line;
  QPushButton* reference_button;

  QLabel* ref_idx_label;
  QSpinBox* ref_idx_box;

  QLabel* min_label;
  QSpinBox* min_box;
  QLabel* max_label;
  QSpinBox* max_box;

  QLabel* stem_label;

  QLabel* gray_label;
  QComboBox* gray_box;
  QLabel* gdi_label;
  QComboBox* gdi_box;
  QLabel* dw_label;
  QComboBox* dw_box;

  QLabel* default_label;
  QComboBox* default_box;
  QLabel* fallback_label;
  QComboBox* fallback_box;
  QComboBox* fallback_hint_or_scale_box;

  QLabel* limit_label;
  QString limit_label_text_with_key;
  QString limit_label_text;
  QSpinBox* limit_box;
  QCheckBox* no_limit_box;
  QString no_limit_box_text_with_key;
  QString no_limit_box_text;

  QLabel* x_increase_label;
  QString x_increase_label_text_with_key;
  QString x_increase_label_text;
  QSpinBox* x_increase_box;
  QCheckBox* no_x_increase_box;
  QString no_x_increase_box_text_with_key;
  QString no_x_increase_box_text;

  QLabel* snapping_label;
  Tooltip_Line_Edit* snapping_line;
  QLabel* family_suffix_label;
  Tooltip_Line_Edit* family_suffix_line;

  QLabel* stem_width_label;
  QString stem_width_label_text_with_key;
  QString stem_width_label_text;
  QSpinBox* stem_width_box;
  QCheckBox* default_stem_width_box;
  QString default_stem_width_box_text_with_key;
  QString default_stem_width_box_text;

  QCheckBox* wincomp_box;
  QCheckBox* adjust_box;
  QCheckBox* hint_box;
  QCheckBox* symbol_box;
  QCheckBox* dehint_box;
  QLabel* info_label;
  QComboBox* info_box;
  QCheckBox* TTFA_box;

  QCheckBox* watch_box;
  QPushButton* run_button;

  QAction* exit_act;
  QAction* about_act;
  QAction* about_Qt_act;

  QLocale* locale;
};

#endif // MAINGUI_H_

// end of maingui.h
