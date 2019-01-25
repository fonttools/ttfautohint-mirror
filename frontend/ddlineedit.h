// ddlineedit.h

// Copyright (C) 2012-2019 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#ifndef DDLINEEDIT_H_
#define DDLINEEDIT_H_

#include <config.h>
#include "ttlineedit.h"

#include <QDragEnterEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

enum Drag_Drop_File_Type
{
  DRAG_DROP_TRUETYPE,
  DRAG_DROP_ANY
};


class Drag_Drop_Line_Edit
: public Tooltip_Line_Edit
{
  Q_OBJECT

  Drag_Drop_File_Type file_type;

public:
  Drag_Drop_Line_Edit(Drag_Drop_File_Type,
                      QWidget* = 0);

  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
};


#endif // DDLINEEDIT_H_

// end of ddlineedit.h
