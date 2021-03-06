/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 * 
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FLITR_TRIANGLE_OVERLAY_H
#define FLITR_TRIANGLE_OVERLAY_H 1

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Vec4d>
#include <osg/Material>
#include <osg/LineWidth>

namespace flitr {

/**
 * Filled or outline triangle with a given center, width and height.
 *
 * The triangle will always be positioned upwards.
 */
class FLITR_EXPORT TriangleOverlay : public GeometryOverlay
{
  public:
    /**
     * Create a new triangle overlay.
     *
     * \param center_x The x position of the triangle center.
     * \param center_y The y position of the triangle center.
     * \param width Width of the base of the triangle.
     * \param height Height of the triangle.
     * \param filled If the triangle should be filled or not.
     */
    TriangleOverlay(double center_x, double center_y, double width, double height, bool filled=false);
    virtual ~TriangleOverlay();

    /**
     * Set the center of the triangle.
     * \param x The x position of the triangle center.
     * \param y The y position of the triangle center.
     */
    void setCenter(double x, double y);
    /**
     * Get the current center of the triangle.
     * \param x The x position of the triangle center.
     * \param y The y position of the triangle center.
     */
    void getCenter(double &x, double &y);

    /**
     * Set the width or base of the triangle.
     */
    void setWidth(double width);
    /**
     * Set the height of the triangle.
     */
    void setHeight(double height);

    /**
     * Get the width or base of the triangle.
     */
    double getWidth() const;
    /**
     * Get the height of the triangle.
     */
    double getHeight() const;

    /**
     * Set the name of the overlay along with the nodes that make up
     * the overlay.
     *
     * This is normally used when geometry intersections are required.
     *
     * \param name New name of the overlay
     */
    virtual void setName(const std::string& name);

  private:
    void makeQuad(bool filled);
    void updateQuad();
    osg::ref_ptr<osg::Vec3Array> _Vertices;
    osg::ref_ptr<osg::Geode> _Geode;
    osg::ref_ptr<osg::Geometry> _Geom;
    osg::ref_ptr<osg::DrawArrays> _DrawArray;

    double _CenterX;
    double _CenterY;
    double _Width;
    double _Height;
};

}

#endif // FLITR_TRIANGLE_OVERLAY_H
