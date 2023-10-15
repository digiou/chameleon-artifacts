
def set_size(width_pt, fraction=1, subplots=(1, 1)):
    """Set figure dimensions to sit nicely in our document.

    Parameters
    ----------
    width_pt: float
            Document width in points
    fraction: float, optional
            Fraction of the width which you wish the figure to occupy
    subplots: array-like, optional
            The number of rows and columns of subplots.
    Returns
    -------
    fig_dim: tuple
            Dimensions of figure in inches
    """
    # Width of figure (in pts)
    fig_width_pt = width_pt * fraction
    # Convert from pt to inches
    inches_per_pt = 1 / 72.27
    # Golden ratio to set aesthetic figure height
    golden_ratio = (5**.5 - 1) / 2
    # Figure width in inches
    fig_width_in = fig_width_pt * inches_per_pt
    # Figure height in inches
    fig_height_in = fig_width_in * golden_ratio * (subplots[0] / subplots[1])
    return fig_width_in, fig_height_in


def closest_time_unit(seconds: float) -> str:
    # Define time units in seconds
    minute = 60
    hour = minute * 60
    day = hour * 24
    week = day * 7
    month = day * 30  # Approximate average month duration
    year = day * 365  # Non-leap year duration

    # Calculate the absolute difference between the input seconds and each time unit
    durations = [
        ("second", 1),
        ("minute", minute),
        ("hour", hour),
        ("day", day),
        ("week", week),
        ("month", month),
        ("year", year)
    ]

    # Find the closest time unit to the given duration
    closest_unit = min(durations, key=lambda x: abs(seconds - x[1]))

    # Calculate the closest number for that unit
    num_units = round(seconds / closest_unit[1])

    return f"{num_units} {closest_unit[0]}{'s' if num_units != 1 else ''}"