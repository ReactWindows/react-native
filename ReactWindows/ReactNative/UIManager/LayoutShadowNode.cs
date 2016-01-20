﻿using Facebook.CSSLayout;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace ReactNative.UIManager
{
    /// <summary>
    /// Shadow node subclass that supplies setters for base view layout
    /// properties such as width, height, flex properties, borders, etc.
    /// </summary>
    public class LayoutShadowNode : ReactShadowNode
    {
        private static readonly ConcurrentDictionary<Type, IReadOnlyDictionary<string, object>> s_enumCache =
            new ConcurrentDictionary<Type, IReadOnlyDictionary<string, object>>();

        // TODO: replace with CSSConstants.Undefined
        private const float Undefined = float.NaN;

        /// <summary>
        /// Set the width of the shadow node.
        /// </summary>
        /// <param name="height">The width.</param>
        [ReactProperty(ViewProperties.Width, DefaultFloat = Undefined)]
        public void SetWidth(float width)
        {
            Width = width;
        }

        /// <summary>
        /// Set the heigth of the shadow node.
        /// </summary>
        /// <param name="height">The height.</param>
        [ReactProperty(ViewProperties.Height, DefaultFloat = Undefined)]
        public void SetHeight(float height)
        {
            Height = height;
        }

        /// <summary>
        /// Sets the left position of the shadow node.
        /// </summary>
        /// <param name="left">The left position.</param>
        [ReactProperty(ViewProperties.Left, DefaultFloat = Undefined)]
        public void SetLeft(float left)
        {
            PositionLeft = left;
        }

        /// <summary>
        /// Sets the top position of the shadow node.
        /// </summary>
        /// <param name="top">The top position.</param>
        [ReactProperty(ViewProperties.Top, DefaultFloat = Undefined)]
        public void SetTop(float top)
        {
            PositionTop = top;
        }

        /// <summary>
        /// Sets the bottom position of the shadow node.
        /// </summary>
        /// <param name="bottom">The bottom position.</param>
        [ReactProperty(ViewProperties.Bottom, DefaultFloat = Undefined)]
        public void SetBottom(float bottom)
        {
            PositionBottom = bottom;
        }

        /// <summary>
        /// Sets the right position of the shadow node.
        /// </summary>
        /// <param name="right">The right position.</param>
        [ReactProperty(ViewProperties.Right, DefaultFloat = Undefined)]
        public void SetRight(float right)
        {
            PositionRight = right;
        }

        /// <summary>
        /// Sets the flex of the shadow node.
        /// </summary>
        /// <param name="flex">The flex value.</param>
        [ReactProperty(ViewProperties.Flex, DefaultFloat = 0f)]
        public void SetFlex(float flex)
        {
            Flex = flex;
        }

        /// <summary>
        /// Sets the flex direction of the shadow node.
        /// </summary>
        /// <param name="flexDirection">The flex direction.</param>
        [ReactProperty(ViewProperties.FlexDirection)]
        public void SetFlexDirection(string flexDirection)
        {
            FlexDirection = flexDirection != null
                ? Parse<CSSFlexDirection>(flexDirection, nameof(flexDirection))
                : CSSFlexDirection.Column;
        }

        /// <summary>
        /// Sets the wrap property on the shadow node.
        /// </summary>
        /// <param name="flexWrap">The wrap.</param>
        [ReactProperty(ViewProperties.FlexWrap)]
        public void SetFlexWrap(string flexWrap)
        {
            Wrap = flexWrap != null
                ? Parse<CSSWrap>(flexWrap, nameof(flexWrap))
                : CSSWrap.NoWrap;
        }

        /// <summary>
        /// Sets the self alignment of the shadow node.
        /// </summary>
        /// <param name="alignSelf">The align self property.</param>
        [ReactProperty(ViewProperties.AlignSelf)]
        public void SetAlignSelf(string alignSelf)
        {
            AlignSelf = alignSelf != null
                ? Parse<CSSAlign>(alignSelf, nameof(alignSelf))
                : CSSAlign.Auto;
        }

        /// <summary>
        /// Sets the item alignment for the shadow node.
        /// </summary>
        /// <param name="alignItems">The item alignment.</param>
        [ReactProperty(ViewProperties.AlignItems)]
        public void SetAlignItems(string alignItems)
        {
            AlignItems = alignItems != null
                ? Parse<CSSAlign>(alignItems, nameof(alignItems))
                : CSSAlign.Stretch;
        }

        /// <summary>
        /// Sets the content justification.
        /// </summary>
        /// <param name="justifyContent">The content justification.</param>
        [ReactProperty(ViewProperties.JustifyContent)]
        public void SetJustifyContent(string justifyContent)
        {
            JustifyContent = justifyContent != null
                ? Parse<CSSJustify>(justifyContent, nameof(justifyContent))
                : CSSJustify.FlexStart;
        }

        /// <summary>
        /// Sets the margins of the shadow node.
        /// </summary>
        /// <param name="index">The spacing type index.</param>
        /// <param name="margin">The margin value.</param>
        [ReactPropertyGroup(
            ViewProperties.Margin,
            ViewProperties.MarginVertical,
            ViewProperties.MarginHorizontal,
            ViewProperties.MarginLeft,
            ViewProperties.MarginRight,
            ViewProperties.MarginTop,
            ViewProperties.MarginBottom,
            DefaultFloat = Undefined)]
        public void SetMargins(int index, float margin)
        {
            SetMargin(ViewProperties.PaddingMarginSpacingTypes[index], margin);
        }

        /// <summary>
        /// Sets the paddings of the shadow node.
        /// </summary>
        /// <param name="index">The spacing type index.</param>
        /// <param name="padding">The padding value.</param>
        [ReactPropertyGroup(
            ViewProperties.Padding,
            ViewProperties.PaddingVertical,
            ViewProperties.PaddingHorizontal,
            ViewProperties.PaddingLeft,
            ViewProperties.PaddingRight,
            ViewProperties.PaddingTop,
            ViewProperties.PaddingBottom,
            DefaultFloat = Undefined)]
        public void SetPaddings(int index, float padding)
        {
            SetPadding(ViewProperties.PaddingMarginSpacingTypes[index], padding);
        }

        /// <summary>
        /// Sets the border width properties for the shadow node.
        /// </summary>
        /// <param name="index">The border spacing type index.</param>
        /// <param name="borderWidth">The border width.</param>
        [ReactPropertyGroup(
            ViewProperties.BorderWidth,
            ViewProperties.BorderLeftWidth,
            ViewProperties.BorderRightWidth,
            ViewProperties.BorderTopWidth,
            ViewProperties.BorderBottomWidth,
            DefaultFloat = Undefined)]
        public void SetBorderWidth(int index, float borderWidth)
        {
            SetBorder(ViewProperties.BorderSpacingTypes[index], borderWidth);
        }

        /// <summary>
        /// Sets the position of the shadow node.
        /// </summary>
        /// <param name="position">The position.</param>
        [ReactProperty(ViewProperties.Position)]
        public void SetPosition(string position)
        {
            PositionType = position != null
                ? Parse<CSSPositionType>(position, nameof(position))
                : CSSPositionType.Relative;
        }

        private static T Parse<T>(string value, string paramName)
        {
            var lookup = s_enumCache.GetOrAdd(
                typeof(T), 
                type => Enum.GetValues(type)
                    .Cast<object>()
                    .ToDictionary(
                        e => e.ToString().ToLowerInvariant(),
                        e => e));

            var result = default(object);
            if (!lookup.TryGetValue(value.ToLowerInvariant().Replace("-", ""), out result))
            {
                throw new ArgumentOutOfRangeException(
                    paramName,
                    string.Format(
                        CultureInfo.InvariantCulture,
                       "Invalid value '{0}' for type '{1}'.",
                       value,
                       typeof(T)));
            }

            return (T)result;
        }
    }
}