//
// mlib/geom2d.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#ifndef __MLIB_GEOM2D_H__
#define __MLIB_GEOM2D_H__

#include <algorithm> // min, max

//
// Point
// 

template<typename T>
struct PointT
{
    T x;
    T y;

                PointT(): x(), y() {}
                PointT(T x_, T y_): x(x_), y(y_) { }
        
                template<typename R>
      explicit  PointT(const PointT<R>& pnt): x(pnt.x), y(pnt.y) {}
        
          bool  IsNull() const { return (x == T()) && (y == T()); }
};

template<typename T>
inline bool operator ==(const PointT<T>& p1, const PointT<T>& p2)
{
    return (p1.x == p2.x) && (p1.y == p2.y);
}

template<typename T>
inline bool operator !=(const PointT<T>& p1, const PointT<T>& p2)
{
    return !(p1 == p2);
}

template<typename T>
inline bool operator <(const PointT<T>& p1, const PointT<T>& p2)
{
    if( p1.x<p2.x )
        return true;
    if( p1.x == p2.x && p1.y<p2.y )
        return true;
    return false; 
}

template<typename T>
inline PointT<T> operator +(const PointT<T>& p1, const PointT<T>& p2)
{
    PointT<T> res = p1;
    res.x += p2.x;
    res.y += p2.y;
    return res;
}

template<typename T>
inline PointT<T> operator -(const PointT<T>& p1, const PointT<T>& p2)
{
    PointT<T> res = p1;
    res.x -= p2.x;
    res.y -= p2.y;
    return res;
}

// IsNull != IsNullSize
template<typename T>
inline bool IsNullSize(const PointT<T>& sz)
{
    return (sz.x == T()) || (sz.y == T());
}

// стандарные типы
typedef PointT<int> Point;
typedef PointT<double> DPoint;

//
// Rect
// 

template<typename T>
struct RectT
{
            RectT(): lft(), top(), rgt(), btm() { }
            RectT(const T x1, const T y1, const T x2, const T y2 )
                : lft(x1), top(y1), rgt(x2), btm(y2) { }

            RectT(const PointT<T>& a, const PointT<T>& b)
                : lft(a.x), top(a.y), rgt(b.x), btm(b.y) { }

            template<typename R>
  explicit  RectT(const RectT<R>& rct)
                : lft(rct.lft), top(rct.top), rgt(rct.rgt), btm(rct.btm) { }


      bool  IsValid() const { return (lft <= rgt) && (top <= btm); }
      bool  IsNull() const { return (lft == rgt) || (top == btm); }
  PointT<T> Size() const { return PointT<T>(Width(), Height()); }
            // сместить на координаты точки
     RectT& operator +=(const PointT<T>& p);
     RectT& operator -=(const PointT<T>& p);
      bool  Contains(const PointT<T>& p) const;

            // пересекаются ли прямоугольники
      bool  Intersects(const RectT<T>& rct) const;

  PointT<T> A() const { return PointT<T>(lft, top); }
  PointT<T> B() const { return PointT<T>(rgt, btm); }

         T  Width()  const { return rgt-lft; }
      void  SetWidth(T wdh)  { rgt = lft + wdh; }
         T  Height() const { return btm-top; }
      void  SetHeight(T hgt) { btm = top + hgt; }

    T  lft;
    T  top;
    T  rgt;
    T  btm;
};

template<typename T>
inline bool operator ==(const RectT<T>& r1, const RectT<T>& r2)
{
    return (r1.lft == r2.lft) && (r1.top == r2.top) &&
           (r1.rgt == r2.rgt) && (r1.btm == r2.btm);
}

template<typename T>
inline bool operator !=(const RectT<T>& r1, const RectT<T>& r2)
{
    return !(r1 == r2);
}

template<typename T>
RectT<T> RectASz(const PointT<T>& a, const PointT<T>& sz)
{
    return RectT<T>(a, a+sz);
}

template<typename T>
RectT<T> Rect0Sz(const PointT<T>& sz)
{
    return RectT<T>(0, 0, sz.x, sz.y);
}

template<typename T>
RectT<T>& ShiftTo00(RectT<T>& rct)
{
    rct.rgt -= rct.lft;
    rct.btm -= rct.top;
    rct.lft = 0;
    rct.top = 0;
    return rct;
}

// сместить на координаты точки
template<typename T>
inline RectT<T> operator +(const RectT<T>& r, const PointT<T>& a);
template<typename T>
inline RectT<T> operator -(const RectT<T>& r, const PointT<T>& a);

// пересечение и объединение прямоугольников
template<typename T>
RectT<T> Union(const RectT<T>& rct1, const RectT<T>& rct2);
template<typename T>
RectT<T> Intersection(const RectT<T>& rct1, const RectT<T>& rct2);

// изменить размеры, не изменяя центр прямоугольника
template<typename T>
RectT<T> EnlargeRect(const RectT<T>& rct, const PointT<T>& pnt);

// изменить размеры, не меняя точки RectT::A()
template<typename T>
void SetSizes(RectT<T>& rct, const PointT<T>& sz);

// изменить положение Rect::A(), не меняя размеров
template<typename T>
void SetLocation(RectT<T>& rct, const PointT<T>& lct);

// стандарные типы
typedef RectT<int> Rect;
typedef RectT<double> DRect;

///////////////////////////////////////////////////////
// Реализация

template<typename T>
RectT<T>& RectT<T>::operator +=(const PointT<T>& p)
{
    lft += p.x;
    rgt += p.x;
    top += p.y;
    btm += p.y;

    return *this;
}

template<typename T>
RectT<T>& RectT<T>::operator -=(const PointT<T>& p)
{
    lft -= p.x;
    rgt -= p.x;
    top -= p.y;
    btm -= p.y;

    return *this;
}

template<typename T>
bool RectT<T>::Contains(const PointT<T>& p) const
{
    return (lft <= p.x) && (top <= p.y) && (rgt > p.x) && (btm > p.y);
}

template<typename T>
bool RectT<T>::Intersects(const RectT<T>& rct) const
{
    return !Intersection(*this, rct).IsNull();
}

template<typename T>
inline RectT<T> operator +(const RectT<T>& r, const PointT<T>& sh)
{
    return RectT<T>(r.A()+sh, r.B()+sh);
}

template<typename T>
inline RectT<T> operator -(const RectT<T>& r, const PointT<T>& sh)
{
    return RectT<T>(r.A()-sh, r.B()-sh);
}

template<typename T>
RectT<T> Union(const RectT<T>& rct1, const RectT<T>& rct2)
{
    RectT<T> res( std::min(rct1.lft, rct2.lft),
                  std::min(rct1.top, rct2.top),
                  std::max(rct1.rgt, rct2.rgt),
                  std::max(rct1.btm, rct2.btm) );
    return res;
}

template<typename T>
RectT<T> Intersection(const RectT<T>& rct1, const RectT<T>& rct2)
{
    RectT<T> res( std::max(rct1.lft, rct2.lft),
                  std::max(rct1.top, rct2.top),
                  std::min(rct1.rgt, rct2.rgt),
                  std::min(rct1.btm, rct2.btm) );
    if( res.lft > res.rgt )
        res.rgt = res.lft; // пусто
    if( res.top > res.btm )
        res.btm = res.top; // пусто
    return res;
}

template<typename T>
RectT<T> EnlargeRect(const RectT<T>& rct, const PointT<T>& pnt)
{
    Point a = rct.A() - pnt;
    Point b = rct.B() + pnt;
    return RectT<T>(a, b);
}

template<typename T>
void SetLocation(RectT<T>& rct, const PointT<T>& lct)
{
    rct = RectASz(lct, rct.Size());
}

template<typename T>
void SetSizes(RectT<T>& rct, const PointT<T>& sz)
{
    rct = RectT<T>(rct.lft,        rct.top, 
                   rct.lft + sz.x, rct.top + sz.y);
}

template<typename T>
PointT<T> FindAForCenteredRect(const PointT<T>& src_sz, const RectT<T>& center_rct)
{
    PointT<T> a;
    a.x = center_rct.lft + (center_rct.Width()  - src_sz.x) / 2;
    a.y = center_rct.top + (center_rct.Height() - src_sz.y) / 2;
    return a;
}

template<typename T>
RectT<T> CenterRect(const RectT<T>& src_rct, const RectT<T>& center_rct, bool is_horiz, bool is_vert)
{
    PointT<T> sz(src_rct.Size());
    PointT<T> a = FindAForCenteredRect(sz, center_rct);
    if( !is_horiz )
        a.x = src_rct.lft;
    if( !is_vert )
        a.y = src_rct.top;

    return RectASz(a, sz);
}


//
//
// 

Rect CeilRect(const DRect& drct);

#endif // #ifndef __MLIB_GEOM2D_H__


