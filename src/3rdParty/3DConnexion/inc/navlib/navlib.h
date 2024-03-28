#ifndef NAVLIB_H_INCLUDED_
#define NAVLIB_H_INCLUDED_
// <copyright file="navlib.h" company="3Dconnexion">
// -------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------
// </copyright>
// <history>
// *************************************************************************************************
// File History
//
// $Id: navlib.h 19940 2023-01-25 07:17:44Z mbonk $
//
// 01/23/14 MSB Initial design
// </history>
// <description>
// *************************************************************************************************
// File Description
//
// This header file describes the interface for navigating in a 2D or 3D view.
//
// *************************************************************************************************
// </description>

#include <navlib/navlib_types.h>
/// <summary>
/// Contains the navigation library API types and functions
/// </summary>
/// <remarks>
/// The functions and types describe an interface for navigating in a 2D or 3D view.
/// <para>
/// In this scheme, a 3dconnexion library is responsible for calculating the position of the camera
/// viewing the scene or object as well as displaying the settings and for supporting user
/// customization.
/// </para>
/// <para>
/// The application is responsible for passing the description of an interface of the 2D/3D view to
/// the 3dconnexion library, and for reacting to the changes to the properties identified by the
/// 3dconnexion library.
/// </para>
/// </remarks>
NAVLIB_BEGIN_

// ************************************************************************************************
// Properties

/// <summary>
/// Property set by the client to indicate that the connection is currently active.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>Clients that have multiple navigation instances open need to inform the navlib which of
/// them is the target for 3D Mouse input. They do this by setting the active_k property of a
/// navigation instance to true.</para>
/// </remarks>
static const property_t active_k = "active";

/// <summary>
/// Property that a client sets to indicate it has keyboard focus.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>Clients that run in container applications via the NLServer proxy set this property to
/// indicate keyboard focus. This will set 3DMouse focus to the navlib connection.</para>
/// </remarks>
static const property_t focus_k = "focus";

/// <summary>
/// Client property that the navlib sets when a motion model is active.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>The motion_k property is set to true by the navlib to notify
/// the client that it is executing a motion model and will update the camera matrix regularly. This
/// is useful for clients that need to run an animation loop. When the navlib has finished
/// navigating the camera position it will set the property to false. By setting motion_k to false,
/// a client may temporarily interrupt a navigation communication and force the Navlib to
/// reinitialize the navigation.</para>
/// </remarks>
static const property_t motion_k = "motion";

/// <summary>
/// Specifies the transform from the client's coordinate system to the navlib coordinate system.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="matrix_t"/> and <see cref="propertyType_t"/> is
/// <see cref="matrix_type"/>.</para>
/// <para>The Navigation Library coordinate system is Y up, X to the right and Z out of the screen.
/// This property is queried directly after new navigation instance is created. This allows the
/// client to specify the other properties using the coordinate system used in the client. For the
/// keep Y up ('Lock Horizon') algorithm to work correctly a non-identity matrix needs to be
/// specified whenever the ground plane is not the X-Z plane.</para>
/// </remarks>
static const property_t coordinate_system_k = "coordinateSystem";

/* Frame properties*/
/// <summary>
/// Specifies the begin and end of a navigation transaction.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="long"/> and <see cref="propertyType_t"/> is
/// <see cref="long_type"/>.</para>
/// <para>The Navigation Library can set more than one client property for a single navigation
/// frame. For example when navigating in an orthographic projection possibly both the view affine
/// and extents will be modified depending on the 3DMouse input. The Navigation Library sets the
/// transaction_k property to a value >0 at the beginning of a navigation frame and to 0 at the end.
/// Clients that need to actively refresh the view can trigger the refresh when the value is set to
/// 0.</para>
/// </remarks>
static const property_t transaction_k = "transaction";

/// <summary>
/// Specifies the time stamp of an animation frame in milliseconds.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="double"/> and <see cref="propertyType_t"/> is
/// <see cref="double_type"/>.</para>
/// <para>When the frame_timing_source_k property is set to 1, the client initiates a frame
/// transaction by informing the Navigation Library of the frame time. When the value is 0, the
/// Navigation Library attempts to synchronize the frames to the monitor vertical blanking
/// rate.</para>
/// </remarks>
static const property_t frame_time_k = "frame.time";

/// <summary>
/// Specifies the source of the frame timing.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="long"/> and <see cref="propertyType_t"/> is
/// <see cref="long_type"/>.</para>
/// <para>By setting the frame_timing_source_k property to 1, the client application informs the
/// Navigation Library that the client has an animation loop and will be the source of the frame
/// timing.</para>
/// </remarks>
static const property_t frame_timing_source_k = "frame.timingSource";

/// <summary>
/// Specifies whether a device is present
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>Currently this always returns true.</para>
/// </remarks>
static const property_t device_present_k = "device.present";

/// <summary>
/// Defines a set of commands.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="SiActionNode_t"/>* and <see cref="propertyType_t"/> is
/// <see cref="actionnodeexptr_type"/>.</para>
/// <para>Command sets can be considered to be button banks. The set can be either the complete list
/// of commands that are available in the application or a single set of commands for a specific
/// application context. The navlib will not query the application for this property. It is the
/// responsibility of the application to update this property when commands are to be made available
/// to the user.</para>
/// </remarks>
static const property_t commands_tree_k = "commands.tree";

/// <summary>
/// The active command.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="string_t"/> and <see cref="propertyType_t"/> is
/// <see cref="string_type"/>.</para>
/// <para>When the user presses a 3DMouse button that has been assign an application command
/// exposed by the commands_tree_k property, the navlib will write this property. The string value
/// will be the corresponding id passed in the commands_tree_k property. Generally the navlib will
/// set this property to an empty string when the corresponding button has been released.</para>
/// </remarks>
static const property_t commands_activeCommand_k = "commands.activeCommand";

/// <summary>
/// Specifies the active set of commands.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="string_t"/> and <see cref="propertyType_t"/> is
/// <see cref="string_type"/>.</para>
/// <para>In applications that have exposed multiple command sets this property needs to be set to
/// define the command set that is active. The navlib will not query the application for this
/// property. It is the responsibility of the application to update this property when the set of
/// commands need to be changed. Normally this will be due to change in application state and may
/// correspond to a menu/toolbar change. If only a single set of commands has been defined, this
/// property defaults to that set.</para>
/// </remarks>
static const property_t commands_activeSet_k = "commands.activeSet";

/// <summary>
/// Specifies an array of images for the 3Dconnexion UI.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="imagearray_t"/> and <see cref="propertyType_t"/> is
/// <see cref="imagearray_type"/>.</para>
/// <para>An image with the same <see cref="SiImage_t.id"/> as a command
/// <see cref="SiActionNodeEx_t.id"/> will be associated with that command in the 3Dconnexion
/// UI.</para>
/// </remarks>
static const property_t images_k = "images";

/* view properties */
/// <summary>
/// Specifies the transformation matrix of the view camera.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="matrix_t"/> and <see cref="propertyType_t"/> is
/// <see cref="matrix_type"/>.</para>
/// <para>This matrix specifies the camera to world transformation of the view. That is,
/// transforming the position (0, 0, 0) by this matrix yields the position of the camera in world
/// coordinates. The navlib will, generally, query this matrix at the beginning of a navigation
/// action and then set the property once per frame.</para>
/// </remarks>
static const property_t view_affine_k = "view.affine";

/// <summary>
/// Specifies the plane equation of the construction plane.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="plane_t"/> and <see cref="propertyType_t"/> is
/// <see cref="plane_type"/>.</para>
/// <para>The plane equation is used by the Navigation Library to distinguish views used for
/// construction in orthographic projections: typically the top, right left etc. views. The
/// Navigation Library assumes that when the camera's look-at axis is parallel to the plane normal,
/// the view should not be rotated.</para>
/// </remarks>
static const property_t view_constructionPlane_k = "view.constructionPlane";

/// <summary>
/// Specifies the orthographic extents of the view in camera coordinates.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="box_t"/> and <see cref="propertyType_t"/> is
/// <see cref="box_type"/>.</para>
/// <para>The orthographic extents of the view are returned as a bounding box in world units
/// relative to the camera/view frame. The view frame is a right-handed coordinate system centered
/// on the view with x to the right and y up. The Navigation Library will only access this property
/// if the view is orthographic.</para>
/// </remarks>
static const property_t view_extents_k = "view.extents";

/// <summary>
/// Specifies the vertical field-of-view of a perspective camera/view in radians.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="double"/> and <see cref="propertyType_t"/> is
/// <see cref="double_type"/>.</para>
/// </remarks>
static const property_t view_fov_k = "view.fov";

/// <summary>
/// Specifies the frustum of a perspective camera/view in camera coordinates.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="frustum_t"/> and <see cref="propertyType_t"/> is
/// <see cref="frustum_type"/>.</para>
/// <para>The navlib uses this property to calculate the field-of-view of the perspective camera.
/// The frustum is also used in algorithms that need to determine if the model is currently visible.
/// The navlib will not write to this property. Instead, if necessary, the navlib will write to the
/// <see cref="view_fov_k"/> property and leave the client to change the frustum as it
/// wishes.</para>
/// </remarks>
static const property_t view_frustum_k = "view.frustum";

/// <summary>
/// Specifies whether the projection type of the view/camera is perspective.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>This property defaults to true. If the client does not supply a function for the navlib to
/// query the view's projection (which it will generally do at the onset of motion), then it must
/// set the property in the navlib if the projection is orthographic or when it changes.</para>
/// </remarks>
static const property_t view_perspective_k = "view.perspective";

/// <summary>
/// Specifies the position of the target of the view/camera.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="point_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>The view interest.</para>
/// </remarks>
static const property_t view_target_k = "view.target";

/// <summary>
/// Specifies whether the view can be rotated.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>This property is generally used to differentiate between orthographic 3D views and views
/// that can only be panned and zoomed such as plan views.</para>
/// </remarks>
static const property_t view_rotatable_k = "view.rotatable";

/// <summary>
/// Specifies the distance between the view camera and the object the user is focused on.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="double"/> and <see cref="propertyType_t"/> is
/// <see cref="double_type"/>.</para>
/// <para>This property is used to define the distance to the users object of interest and determines
/// the translation speed sensitivity of the camera or SpaceMouse. </para>
/// </remarks>
static const property_t view_focusDistance_k = "view.focusDistance";

/// <summary>
/// Specifies the orientation of the view designated as the front view.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="matrix_t"/> and <see cref="propertyType_t"/> is
/// <see cref="matrix_type"/>.</para>
/// <para>The Navigation Library will only query the value of this property when the connection is
/// created. It is used to orientate the model to one of the 'Front', 'Back', 'Right', 'Left' etc.
/// views in response to the respective pre-defined view commands. If the orientation of the front
/// view is redefined after the connection is opened by the user, the client application is required
/// to update the property to the new value.</para>
/// </remarks>
static const property_t views_front_k = "views.front";

/// <summary>
/// Specifies the position of the rotation pivot.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="point_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>The Navigation Library will generally set <see cref="pivot_position_k"/> property when
/// navigation ends. The position will depend on which pivot model is being used. The application
/// can set the pivot to a fix position by setting this property. A side effect of the application
/// setting the property is that the <see cref="pivot_user_k"/> property is set to true.</para>
/// </remarks>
static const property_t pivot_position_k = "pivot.position";

/// <summary>
/// Specifies whether the position of the rotation pivot is set by the user.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>With the property set to true, the Navigation Library will disable the internal pivot
/// position algorithms.</para>
/// </remarks>
static const property_t pivot_user_k = "pivot.user";

/// <summary>
/// Specifies whether the rotation pivot widget is visible.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>Set by the Navigation Library when it wants to set the visibility of the pivot used for
/// the 3D navigation. This will be dependent on the user setting for the pivot visibility in the
/// 3Dconnexion Settings configuration and whether the Navigation Library is actively navigating
/// the scene.</para>
/// </remarks>
static const property_t pivot_visible_k = "pivot.visible";

/// <summary>
/// Specifies the origin of the ray used for hit-testing.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="point_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>Set by the Navigation Library. The location is relative to the world coordinate
/// system.</para>
/// </remarks>
static const property_t hit_lookfrom_k = "hit.lookfrom";

/// <summary>
/// Specifies the direction of the ray used for hit-testing.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="vector_t"/> and <see cref="propertyType_t"/> is
/// <see cref="vector_type"/>.</para>
/// <para>Set by the Navigation Library. The direction is relative to the world coordinate
/// system frame.</para>
/// </remarks>
static const property_t hit_direction_k = "hit.direction";

/// <summary>
/// Specifies the diameter of the ray used for hit-testing.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="double"/> and <see cref="propertyType_t"/> is
/// <see cref="double_type"/>.</para>
/// <para>Set by the Navigation Library. This is the diameter of the aperture on the frustum near
/// plane. In a perspective project the ray is a cone.</para>
/// </remarks>
static const property_t hit_aperture_k = "hit.aperture";

/// <summary>
/// Specifies the point of the model that is hit by the ray originating from
/// <see cref="hit_lookfrom_k"/>.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="point_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>This property is queried by the navlib. The navlib will generally calculate if it is
/// possible to hit a part of the model from the <see cref="model_extents_k"/> and
/// <see cref="selection_extents_k"/> properties before setting up the hit-test properties and
/// querying the property. The position is relative to the world coordinate system frame.</para>
/// </remarks>
static const property_t hit_lookat_k = "hit.lookat";

/// <summary>
/// Specifies whether the hit-testing is limited solely to the current selection set.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// </remarks>
static const property_t hit_selectionOnly_k = "hit.selectionOnly";

/// <summary>
/// Specifies the transformation matrix of the selection set.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="matrix_t"/> and <see cref="propertyType_t"/> is
/// <see cref="matrix_type"/>.</para>
/// <para>This matrix specifies the object to world transformation of the selection set. That is,
/// transforming the position (0, 0, 0) by this matrix yields the position of the set in world
/// coordinates. The navlib will, generally, query this matrix at the beginning of a navigation
/// action that involves moving the selection and then set the property once per frame.</para>
/// </remarks>
static const property_t selection_affine_k = "selection.affine";

/// <summary>
/// Specifies whether the selection set is empty.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="bool_t"/> and <see cref="propertyType_t"/> is
/// <see cref="bool_type"/>.</para>
/// <para>When true, nothing is selected.</para>
/// </remarks>
static const property_t selection_empty_k = "selection.empty";

/// <summary>
/// Specifies the bounding box of the selection set.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="box_t"/> and <see cref="propertyType_t"/> is
/// <see cref="box_type"/>.</para>
/// <para>The extents of the selection are returned as a bounding box in world coordinates. The
/// Navigation Library will only access this property if the <see cref="selection_empty_k"/>
/// property is false.</para>
/// </remarks>
static const property_t selection_extents_k = "selection.extents";

/// <summary>
/// Specifies the bounding box of the model.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="box_t"/> and <see cref="propertyType_t"/> is
/// <see cref="box_type"/>.</para>
/// </remarks>
static const property_t model_extents_k = "model.extents";

/// <summary>
/// Specifies the plane equation of the floor. /// </summary>
/// <remarks>
/// <para>The type is <see cref="plane_t"/> and <see cref="propertyType_t"/> is
/// <see cref="plane_type"/>.</para>
/// <para>The plane equation is used by the Navigation Library to determine the floor for the
/// walk navigation mode, where the height of the eye is fixed to 1.5m above the floor plane.
/// The floor need not be parallel to the world ground plane.</para>
/// Introduced in 3DxWare 10 version 10.8.12.</remarks>
static const property_t model_floorPlane_k = "model.floorPlane";

/// <summary>
/// Specifies the length of the model/world units in meters. /// </summary>
/// <remarks>
/// <para>The type is <see cref="double"/> and <see cref="propertyType_t"/> is
/// <see cref="double_type"/>.</para>
/// <para>The conversion factor is used by the Navigation Library to calculate the height above the
/// floor in walk mode and the speed in the first person motion model.</para>
/// Introduced in 3DxWare 10 version 10.8.12.</remarks>
static const property_t model_unitsToMeters_k = "model.unitsToMeters";

/// <summary>
/// Specifies the position of the mouse cursor. on the projection plane in world coordinates.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="point_t"/> and <see cref="propertyType_t"/> is
/// <see cref="point_type"/>.</para>
/// <para>The position of the mouse cursor is in world coordinates on the projection plane. For a
/// perspective projection the Navigation Library uses the near clipping as the projection plane.
/// In OpenGL the position would typically be retrieved using gluUnProject with winZ set to
/// 0.0.</para>
/// </remarks>
static const property_t pointer_position_k = "pointer.position";

/// <summary>
/// V3DK press event.
/// </summary>
static const property_t events_keyPress_k = "events.keyPress";

/// <summary>
/// V3DK release event.
/// </summary>
static const property_t events_keyRelease_k = "events.keyRelease";

/// <summary>
/// Used to query and apply settings in the 3Dconnexion Settings UI.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="string_t"/> and <see cref="propertyType_t"/> is
/// <see cref="string_type"/>.</para>
/// <para>The property settings_k does not actually exist in the Navigation Library. To read or
/// write a property to the application profile, the settings_k needs to be appended with "." and
/// the name of the profile property.<example>"settings.MoveObjects" is used to read or write the
/// value of the "MoveObjects" property in the profile settings.</example></para>
/// </remarks>
static const property_t settings_k = "settings";

/// <summary>
/// Specifies the change revision of the profile settings.
/// </summary>
/// <remarks>
/// <para>The type is <see cref="long"/> and <see cref="propertyType_t"/> is
/// <see cref="long_type"/>.</para>
/// <para>This property is incremented when the settings changed. The value is only valid for the
/// current connection to the Navigation Library and is not persistent over multiple sessions. If
/// the client needs to know the value of a 3Dconnexion profile setting it should re-read the value
/// when settings_changed_k is changed.</para>
/// </remarks>
static const property_t settings_changed_k = "settings.changed";

// Workaround for error C2099: initializer is not a constant when compiling .c
#if __cplusplus
/// <summary>
/// Defines the type of a property and the access required of the client application.
/// </summary>
static const propertyDescription_t propertyDescription[] = {
    /* property, type, required client access */
    {active_k, bool_type, eno_access},
    {focus_k, bool_type, eno_access},
    {motion_k, bool_type, ewrite_access},
    {coordinate_system_k, matrix_type, eread_access},
    {device_present_k, bool_type, eno_access},
    {events_keyPress_k, long_type, ewrite_access},
    {events_keyRelease_k, long_type, ewrite_access},

    /* frame properties*/
    {transaction_k, long_type, ewrite_access},
    {frame_time_k, double_type, eread_access},
    {frame_timing_source_k, long_type, eread_access},

    /* view properties */
    {view_affine_k, matrix_type, eread_write_access},
    {view_constructionPlane_k, plane_type, eread_access},
    {view_extents_k, box_type, eread_write_access},
    {view_fov_k, float_type, eread_write_access},
    {view_frustum_k, frustum_type, eread_access},
    {view_perspective_k, bool_type, eread_access},
    {view_rotatable_k, bool_type, eread_access},
    {view_target_k, point_type, eread_access},
    {view_focusDistance_k, float_type, eread_access},

    /* views properties*/
    {views_front_k, matrix_type, eread_access},

    /* pivot properties */
    {pivot_position_k, point_type, eread_write_access},
    {pivot_user_k, bool_type, eno_access},
    {pivot_visible_k, bool_type, ewrite_access},

    /* hit-test properties */
    {hit_lookfrom_k, point_type, ewrite_access},
    {hit_direction_k, vector_type, ewrite_access},
    {hit_aperture_k, float_type, ewrite_access},
    {hit_lookat_k, point_type, eread_access},
    {hit_selectionOnly_k, bool_type, ewrite_access},

    /* selection properties */
    {selection_affine_k, matrix_type, eread_write_access},
    {selection_empty_k, bool_type, eread_access},
    {selection_extents_k, box_type, eread_access},

    /* model properties */
    {model_extents_k, box_type, eread_access},
    {model_floorPlane_k, plane_type, eread_access},
    {model_unitsToMeters_k, float_type, eread_access},

    /* pointer (cursor) properties */
    {pointer_position_k, point_type, eread_access},

    /* commands properties */
    {commands_tree_k, actionnodeexptr_type, eno_access},
    {commands_activeSet_k, string_type, eno_access},
    {commands_activeCommand_k, string_type, ewrite_access},

    /* images properties*/
    {images_k, imagearray_type, eno_access},

    /* settings property*/
    {settings_k, string_type, eno_access},
    {settings_changed_k, long_type, ewrite_access}};
#endif

/**********************************************************************************************
Functions exported from the library
 **********************************************************************************************/

/// <summary>
/// Creates a new navigation instance.
/// </summary>
/// <remarks>The client specifies the name of the instance and the properties that are available
/// for querying and updating by the navigation framework.</remarks>
/// <param name="pnh">A pointer to a <see cref="nlHandle_t"/> for the new navigation
/// instance.</param>
/// <param name="appname">The name of the application.</param>
/// <param name="property_accessors">An array of <see cref="accessor_t"/> structures containing the
/// property name, accessor and mutator functions that the client exposes to the navigation
/// instance.</param>
/// <param name="accessor_count">The number of <see cref="accessor_t"/> entries passed in the
/// property_accessors parameter.</param>
/// <param name="options">Pointer to a <see cref="nlCreateOptions_t"/>. This parameter is optional
/// and may be null.</param>
/// <returns>0 on success or a navlib error, see <see cref="navlib_errc::navlib_errc_t"/> and
/// <see cref="make_result_code"/>.</returns>
NAVLIB_DLLAPI_ long __cdecl NlCreate(nlHandle_t *pnh, const char *appname,
                                     const accessor_t property_accessors[], size_t accessor_count,
                                     const nlCreateOptions_t *options);

/// <summary>
/// Closes an open navigation instance handle and destroys the navigation instance.
/// </summary>
/// <param name="nh">A valid <see cref="nlHandle_t"/> of an open navigation instance.</param>
/// <returns>0 if the function succeeds, otherwise a navlib error, see
/// <see cref="navlib_errc::navlib_errc_t"/> and <see cref="make_result_code"/>.</returns>
NAVLIB_DLLAPI_ long __cdecl NlClose(nlHandle_t nh);

/// <summary>
/// Read the value of a property cached in the navlib.
/// </summary>
/// <param name="nh">The <see cref="nlHandle_t"/> of the open navigation instance.</param>
/// <param name="name">The name of the property whose value is being queried.</param>
/// <param name="value">A pointer to a <see cref="value_t"/> that contains the property value when
/// the function returns.</param>
/// <returns>0 if the function succeeds, otherwise a navlib error, see
/// <see cref="navlib_errc::navlib_errc_t"/> and <see cref="make_result_code"/>.</returns>
NAVLIB_DLLAPI_ long __cdecl NlReadValue(nlHandle_t nh, property_t name, value_t *value);

/// <summary>
/// Write the value for a property to the navlib.
/// </summary>
/// <param name="nh">The <see cref="nlHandle_t"/> of the open navigation instance.</param>
/// <param name="name">The name of the property whose value is to be written.</param>
/// <param name="value">A pointer to a <see cref="value_t"/> that contains the new property
/// value.</param>
/// <returns>0 if the function succeeds, otherwise a navlib error, see
/// <see cref="navlib_errc::navlib_errc_t"/> and <see cref="make_result_code"/>.</returns>
NAVLIB_DLLAPI_ long __cdecl NlWriteValue(nlHandle_t nh, property_t name, const value_t *value);

/// <summary>
/// Query the type of a navlib property.
/// </summary>
/// <param name="name">The name of the property whose type is to be queried.</param>
/// <returns>One of the <see cref="propertyTypes"/> values.</returns>
NAVLIB_DLLAPI_ propertyType_t __cdecl NlGetType(property_t name);

NAVLIB_END_

#endif // NAVLIB_H_INCLUDED_
